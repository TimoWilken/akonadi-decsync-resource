/* -*- compile-command: "cd .. && cmake . -DCMAKE_BUILD_TYPE=debugfull && make"; c-basic-offset: 4; eval: (c-set-offset 'arglist-intro '+) -*-
 *
 * Copyright (C) 2020 by Timo Wilken <timo.21.wilken@gmail.com>
 *
 * This program is free software; you can redistribute iter and/or
 * modify iter under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that iter will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "decsyncresource.h"

#include "settings.h"
#include "settingsadaptor.h"
#include "debug.h"

#include <QDBusConnection>
#include <QUrl>
#include <QFileDialog>
#include <QHostInfo>

#include <KLocalizedString>

#include <json/json.h>

DecSyncResource::DecSyncResource(const QString &id)
    : ResourceBase(id)
{
    new SettingsAdaptor(Settings::self());
    QDBusConnection::sessionBus().registerObject(
        QStringLiteral("/Settings"),
        Settings::self(),
        QDBusConnection::ExportAdaptors);

    // UNIX-like, then Windows
    QString userName = qEnvironmentVariable(
        "USER", qEnvironmentVariable("USERNAME", QStringLiteral("nobody")));

    // Our DecSync client ID is <hostname>-akonadi-<user>. DecSyncCC (phone
    // client) seems to call itself <hostname>-DecSyncCC, while Evolution calls
    // itself <hostname>-Evolution-<N> for some integer N (what is this number?)
    QString unsanitizedClientID =
        QHostInfo::localHostName() + QStringLiteral("-akonadi-") + userName;
    this->clientID = QString::fromUtf8(
        unsanitizedClientID
        .toUtf8()
        .toPercentEncoding(QByteArrayLiteral(""), QByteArrayLiteral("~")));

    qCDebug(log_decsyncresource, "Resource started");
}

DecSyncResource::~DecSyncResource()
{
}

QString DecSyncResource::getLatestClientID(Akonadi::Collection collection)
{
    QString latestClientID;
    QDateTime latestUpdate = QDateTime::fromSecsSinceEpoch(0, Qt::UTC);

    QDirIterator infoIter(collection.remoteId() + QStringLiteral("/info"),
                          QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (infoIter.hasNext()) {
        infoIter.next();
        if (infoIter.fileName() == this->clientID) {
            continue;
        }

        QFile file(infoIter.filePath() + QStringLiteral("/last-stored-entry"));
        file.open(QIODevice::ReadOnly);
        QDateTime update = QDateTime::fromString(
            QString::fromUtf8(file.readAll()).trimmed(), Qt::ISODate);
        if (update > latestUpdate) {
            latestUpdate = update;
            latestClientID = infoIter.fileName();
        }
        file.close();
    }

    return latestClientID;
}

QMap<QString, QString> readEntry(QString entryFilePath)
{
    QMap<QString, QString> dict;
    // Json::CharReader jsonReader = Json::CharReaderBuilder().newCharReader();
    QFile file(entryFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            Json::Value array;
            try {
                std::istringstream(stream.readLine().toStdString()) >> array;
                dict.insert(QString::fromStdString(array[1u].asString()),
                            QString::fromStdString(array[2u].asString()));
            } catch (const std::exception &e) {
                qCWarning(log_decsyncresource,
                          "error parsing JSON line from %s (line ignored): %s",
                          entryFilePath.toStdString().data(), e.what());
            }
        }
        file.close();
    }
    if (file.error()) {
        qCWarning(log_decsyncresource, "error 0x%x reading file %s",
                  file.error(), entryFilePath.toStdString().data());
    }
    return dict;
}

calendar_t DecSyncResource::getCalendarInfo(Akonadi::Collection calendar)
{
    QMap<QString, QString> info = readEntry(
        calendar.remoteId() + QStringLiteral("/stored-entries/") +
        this->getLatestClientID(calendar) + QStringLiteral("/info"));

    calendar_t result;
    result.name = info.value(QStringLiteral("name"),
                             QStringLiteral("<unnamed calendar>"));
    result.color = info.value(QStringLiteral("color"),
                              QStringLiteral("#ffff00"));
    return result;
}

void DecSyncResource::retrieveCollections()
{
    // TODO: this method is called when Akonadi wants to have all the
    // collections your resource provides.
    // Be sure to set the remote ID and the content MIME types

    const QUrl calendarDir = QUrl::fromLocalFile(
        Settings::self()->decSyncDirectory() + QStringLiteral("/calendars"));

    Akonadi::Collection calendars;
    calendars.setParentCollection(Akonadi::Collection::root());
    calendars.setRemoteId(calendarDir.url());
    calendars.setName(name());
    calendars.setRights(Akonadi::Collection::Right::ReadOnly);

    const QStringList calendarMimes(QStringLiteral("text/calendar"));
    calendars.setContentMimeTypes(calendarMimes);

    Akonadi::Collection::List collections = { calendars };

    QDirIterator iter(calendarDir.path(), QStringList(QStringLiteral("colID*")),
                      QDir::Dirs | QDir::NoDotAndDotDot);

    while (iter.hasNext()) {
        iter.next();
        Akonadi::Collection calendar;
        calendar.setParentCollection(calendars);
        calendar.setRemoteId(calendarDir.url() + QStringLiteral("/") + iter.fileName());
        // TODO: get name from calendars/colID.../stored-entries/<latest>/info
        calendar.setName(iter.fileName());
        calendar.setContentMimeTypes(calendarMimes);
        calendar.setRights(Akonadi::Collection::Right::ReadOnly);
        collections << calendar;
    }

    collectionsRetrieved(collections);
}

void DecSyncResource::retrieveItems(const Akonadi::Collection &collection)
{
    // TODO: this method is called when Akonadi wants to know about all the
    // items in the given collection. You can but don't have to provide all the
    // data for each item, remote ID and MIME type are enough at this stage.
    // Depending on how your resource accesses the data, there are several
    // different ways to tell Akonadi when you are done.

    Q_UNUSED(collection);

    Akonadi::Item::List items;

    const QStringList calendarMimes(QStringLiteral("text/calendar"));

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

void DecSyncResource::aboutToQuit()
{
    // TODO: any cleanup you need to do while there is still an active
    // event loop. The resource will terminate after this method returns
}

void DecSyncResource::configure(WId windowId)
{
    // TODO: this method is usually called when a new resource is being
    // added to the Akonadi setup. You can do any kind of user interaction here,
    // e.g. showing dialogs.
    // The given window ID is usually useful to get the correct
    // "on top of parent" behavior if the running window manager applies any kind
    // of focus stealing prevention technique
    //
    // If the configuration dialog has been accepted by the user by clicking Ok,
    // the signal configurationDialogAccepted() has to be emitted, otherwise, if
    // the user canceled the dialog, configurationDialogRejected() has to be emitted.
    Q_UNUSED(windowId);

    const QString
        oldPath = Settings::self()->decSyncDirectory(),
        newPath = QFileDialog::getExistingDirectory(
            nullptr,
            i18nc("@title:window", "Select DecSync folder"),
            QUrl::fromLocalFile(oldPath.isEmpty() ? QDir::homePath() : oldPath).path());

    if (newPath.isEmpty() || oldPath == newPath) {
        configurationDialogRejected();
        return;
    }

    Settings::self()->setDecSyncDirectory(newPath);
    Settings::self()->save();
    synchronize();
    configurationDialogAccepted();
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
    // TODO: this method is called when somebody else, e.g. a client application,
    // has created an item in a collection managed by your resource.

    Q_UNUSED(item);
    Q_UNUSED(collection);
}

void DecSyncResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    // TODO: this method is called when somebody else, e.g. a client application,
    // has changed an item managed by your resource.

    Q_UNUSED(item);
    Q_UNUSED(parts);
}

void DecSyncResource::itemRemoved(const Akonadi::Item &item)
{
    // TODO: this method is called when somebody else, e.g. a client application,
    // has deleted an item managed by your resource.

    Q_UNUSED(item);
}

AKONADI_RESOURCE_MAIN(DecSyncResource)
