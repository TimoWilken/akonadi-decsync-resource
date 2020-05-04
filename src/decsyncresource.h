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

#ifndef DECSYNCRESOURCE_H
#define DECSYNCRESOURCE_H

#include <ResourceBase>

#define MAX_COLLECTIONS      256
#define FRIENDLY_NAME_LENGTH 256
#define APPID_LENGTH         256
#define PATHSEP              '/'

const QList<const char*> COLLECTION_TYPES { "calendars", "contacts", "feeds" };

/**
 * An instance of this struct is passed to the function called by libdecsync
 * when it reads an item. This lets our function create an item in the correct
 * Akonadi::Item::List and with the correct MIME type.
 */
struct ItemListAndMime {
    Akonadi::Item::List &items;
    const QString mime;
    ItemListAndMime(Akonadi::Item::List &list, const QString mimetype)
        : items{list}, mime{mimetype} {}
};

class DecSyncResource : public Akonadi::ResourceBase,
                        public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT

public:
    explicit DecSyncResource(const QString &id);
    ~DecSyncResource() override;

public Q_SLOTS:
    void configure(WId windowId) override;

protected:

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;

protected:
    using Akonadi::ResourceBase::retrieveItems;

    void aboutToQuit() override;

    void itemAdded(const Akonadi::Item &item,
                   const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item,
                     const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void collectionAdded(const Akonadi::Collection &collection,
                         const Akonadi::Collection &parent) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

    // Don't override the Akonadi::AgentBase::Observer version of this method,
    // which has a different signature to the ObserverV2 method.
    using Akonadi::AgentBase::ObserverV2::collectionChanged;
    void collectionChanged(const Akonadi::Collection &collection,
                           const QSet<QByteArray> &changedAttributes) override;

private:
    char appId[APPID_LENGTH];
};

#endif
