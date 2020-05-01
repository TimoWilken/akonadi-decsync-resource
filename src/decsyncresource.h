/* -*- c-basic-offset: 4; -*-
 *
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

struct calendar_t {
    QString name;
    QString color;
};

class DecSyncResource : public Akonadi::ResourceBase,
                        public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    explicit DecSyncResource(const QString &id);
    ~DecSyncResource() override;

public Q_SLOTS:
    void configure(WId windowId) override;

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;

protected:
    void aboutToQuit() override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    // TODO: override collection{Added,Changed,Removed}

private:
    QString clientID;
    QString getLatestClientID(Akonadi::Collection collection);
    calendar_t getCalendarInfo(Akonadi::Collection calendar);
};

#endif
