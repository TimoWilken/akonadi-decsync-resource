/*
 * Copyright (C) 2020 by Timo Wilken <timo.21.wilken@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "decsyncresource.h"

#include "../build/src/settings.h"
#include "../build/src/settingsadaptor.h"
#include "../build/src/debug.h"

#include <QDBusConnection>
#include <QUrl>
#include <QFileDialog>
#include <QHostInfo>

#include <KLocalizedString>

#include <libdecsync.h>

DecSyncResource::DecSyncResource(const QString &id)
    : ResourceBase(id)
{
    new SettingsAdaptor(Settings::self());
    QDBusConnection::sessionBus().registerObject(
        QStringLiteral("/Settings"),
        Settings::self(),
        QDBusConnection::ExportAdaptors);

    setNeedsNetwork(false);

    const int versionStatus = decsync_check_decsync_info(
        Settings::self()->decSyncDirectory().toUtf8().constData());
    setOnline(!versionStatus);
    if (versionStatus) {
        const char* errorMessage =
            versionStatus == 1 ? "libdecsync: %s: found invalid .decsync-version" :
            versionStatus == 2 ? "libdecsync: %s: unsupported version" :
            "libdecsync: %s: unknown error";
        Q_EMIT status(Akonadi::AgentBase::Status::Broken, QString::fromUtf8(errorMessage));
        qCCritical(log_decsyncresource, "%s", errorMessage);
        setTemporaryOffline(60);
    }

    decsync_get_app_id("akonadi", this->appId, APPID_LENGTH);
    qCDebug(log_decsyncresource, "resource started with app ID %s", this->appId);
}

/**
 * This method is usually called when a new resource is being added to the
 * Akonadi setup. You can do any kind of user interaction here, e.g. showing
 * dialogs.
 *
 * The given window ID is usually useful to get the correct "on top of parent"
 * behavior if the running window manager applies any kind of focus stealing
 * prevention technique.
 *
 * If the configuration dialog has been accepted by the user by clicking Ok, the
 * signal configurationDialogAccepted() has to be emitted, otherwise, if the
 * user canceled the dialog, configurationDialogRejected() has to be emitted.
*/
void DecSyncResource::configure(WId windowId)
{
    Q_UNUSED(windowId);

    const QString oldPath = Settings::self()->decSyncDirectory();
    const QString newPath = QFileDialog::getExistingDirectory(
        nullptr,
        i18nc("@title:window", "Select DecSync folder"),
        QUrl::fromLocalFile(oldPath.isEmpty() ? QDir::homePath() : oldPath).path());

    if (newPath.isEmpty() || oldPath == newPath ||
        decsync_check_decsync_info(qUtf8Printable(newPath)) != 0) {
        configurationDialogRejected();
        return;
    }

    Settings::self()->setDecSyncDirectory(newPath);
    Settings::self()->save();
    synchronize();
    configurationDialogAccepted();
}

DecSyncResource::~DecSyncResource() {}

void DecSyncResource::aboutToQuit()
{
    // Any cleanup you need to do while there is still an active event loop. The
    // resource will terminate after this method returns.
}

void DecSyncResource::retrieveCollections()
{
    Akonadi::Collection::List collections;

    for (QMap<const char*, QStringList>::const_iterator type =
             COLLECTION_TYPES_AND_MIMETYPES.constBegin();
         type != COLLECTION_TYPES_AND_MIMETYPES.constEnd(); ++type) {
#define MAX_COLLECTIONS 256
        char names[MAX_COLLECTIONS][256];
        int collectionsFound = decsync_list_decsync_collections(
            qUtf8Printable(Settings::self()->decSyncDirectory()),
            type.key(), (const char**)names, MAX_COLLECTIONS);
#undef MAX_COLLECTIONS

        for (int i = 0; i < collectionsFound; ++i) {
            Decsync sync;
            if (int error = decsync_new(
                    &sync, qUtf8Printable(Settings::self()->decSyncDirectory()),
                    type.key(), names[i], this->appId)) {
                qCWarning(log_decsyncresource,
                          "failed to initialize DecSync %s collection %s: error %d",
                          type.key(), names[i], error);
                continue;
            }

            Akonadi::Collection coll;
            coll.setParentCollection(Akonadi::Collection::root());
            coll.setRemoteId(QString::fromUtf8(type.key()) + PATHSEP + QString::fromUtf8(names[i]));
            coll.setContentMimeTypes(type.value());
            coll.setRights(Akonadi::Collection::Right::ReadOnly);

#define FRIENDLY_NAME_LENGTH 256
            char friendlyName[FRIENDLY_NAME_LENGTH];
            decsync_get_static_info(
                qUtf8Printable(Settings::self()->decSyncDirectory()),
                type.key(), names[i], "\"name\"", friendlyName, FRIENDLY_NAME_LENGTH);
#undef FRIENDLY_NAME_LENGTH

            coll.setName(QString::fromUtf8(friendlyName));

            collections << coll;

            decsync_free(sync);
        }
    }

    collectionsRetrieved(collections);
}

void onEntryUpdate(const char** path, const int len, const char* datetime,
                   const char* key, const char* value, void* extra)
{
    qCDebug(log_decsyncresource,
            "got update notification, extra=%p path[0/%d]=%s datetime=%s key=%s value=%s",
            extra, len, len ? path[0] : "<empty>", datetime, key, value);

    QStringList pathComponents;
    for (int i = 0; i < len; ++i) {
        pathComponents << QString::fromUtf8(path[i]);
    }

    ItemListAndMime info = *static_cast<ItemListAndMime*>(extra);
    Akonadi::Item item;
    item.setRemoteId(pathComponents.join(PATHSEP));
    item.setMimeType(info.mime);
    item.setPayloadFromData(value);
    info.items << item;
}

void DecSyncResource::retrieveItems(const Akonadi::Collection &collection)
{
    // This method is called when Akonadi wants to know about all the items in
    // the given collection. You can but don't have to provide all the data for
    // each item, remote ID and MIME type are enough at this stage.

    const char* collType = collection.remoteId().section(PATHSEP, 0, 0).toUtf8().constData();
    const char* collName = collection.remoteId().section(PATHSEP, 1, 1).toUtf8().constData();

    Decsync sync;
    if (int error = decsync_new(
            &sync, qUtf8Printable(Settings::self()->decSyncDirectory()),
            collType, collName, this->appId)) {
        Q_EMIT status(Akonadi::AgentBase::Status::Broken,
                      QStringLiteral("failed to initialize DecSync collection"));
        qCWarning(log_decsyncresource,
                  "failed to initialize DecSync %s collection %s: error %d",
                  collType, collName, error);
        return;
    }

    decsync_add_listener(sync, {}, 0, onEntryUpdate);
    decsync_init_stored_entries(sync);

    Akonadi::Item::List items;
    ItemListAndMime info(items, COLLECTION_TYPES_AND_MIMETYPES[collType][0]);
    decsync_execute_all_stored_entries_for_path_prefix(sync, {}, 0, (void*)&info);

    decsync_free(sync);
    itemsRetrieved(items);
}

bool DecSyncResource::retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts)
{
    // TODO: Does this work?
    // retrieveItems(const Collection&) should have populated all Items already!
    Q_UNUSED(parts);
    itemsRetrieved(items);
    return true;
}

/*
 * Note that these three functions don't get the full payload of the items by default,
 * you need to change the item fetch scope of the change recorder to fetch the full
 * payload. This can be expensive with big payloads, though.
 *
 * Once you have handled changes in itemAdded() and itemChanged(), call changeCommitted().
 * Once you have handled changes in itemRemoved(), call changeProcessed();
 * These methods are called whenever a local item related to this resource is
 * added, modified or deleted. They are only called if the resource is online, otherwise
 * all changes are recorded and replayed as soon the resource is online again.
 */

void DecSyncResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(item);
    Q_UNUSED(collection);
}

void DecSyncResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(item);
    Q_UNUSED(parts);
}

void DecSyncResource::itemRemoved(const Akonadi::Item &item)
{
    Q_UNUSED(item);
}

void DecSyncResource::collectionAdded(const Akonadi::Collection &collection,
                                      const Akonadi::Collection &parent)
{
    Q_UNUSED(collection);
    Q_UNUSED(parent);
}

void DecSyncResource::collectionChanged(const Akonadi::Collection &collection,
                                        const QSet<QByteArray> &changedAttributes)
{
    Q_UNUSED(collection);
    Q_UNUSED(changedAttributes);
}

void DecSyncResource::collectionRemoved(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
}

AKONADI_RESOURCE_MAIN(DecSyncResource)
