/*
    Copyright (C) 2014 Aseman
    http://aseman.co

    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This project is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "telegramcontactsmodel.h"
#include "telegramqml.h"
#include "objects/types.h"

#include <telegram.h>
#include <QPointer>

class TelegramContactsModelPrivate
{
public:
    QPointer<TelegramQml> telegram;
    QList<qint64> contacts;
    bool initializing;
};

TelegramContactsModel::TelegramContactsModel(QObject *parent) :
    TgAbstractListModel(parent)
{
    p = new TelegramContactsModelPrivate;
    p->telegram = 0;
    p->initializing = false;
}

TelegramQml *TelegramContactsModel::telegram() const
{
    return p->telegram;
}

void TelegramContactsModel::setTelegram(TelegramQml *tgo)
{
    TelegramQml *tg = static_cast<TelegramQml*>(tgo);
    if( p->telegram == tg )
        return;
    if(p->telegram)
    {
        disconnect(p->telegram, SIGNAL(contactsChanged()), this, SLOT(contactsChanged()));
        disconnect(p->telegram, SIGNAL(authLoggedInChanged()), this, SLOT(recheck()));
    }

    p->telegram = tg;
    if(p->telegram)
    {
        connect(p->telegram, SIGNAL(contactsChanged()), this, SLOT(contactsChanged()));
        connect(p->telegram, SIGNAL(authLoggedInChanged()), this, SLOT(recheck()), Qt::QueuedConnection);
    }

    refresh();
    Q_EMIT telegramChanged();
}

qint64 TelegramContactsModel::id(const QModelIndex &index) const
{
    int row = index.row();
    return p->contacts.at(row);
}

int TelegramContactsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return p->contacts.count();
}

QVariant TelegramContactsModel::data(const QModelIndex &index, int role) const
{
    QVariant res;
    const qint64 key = id(index);
    switch( role )
    {
    case ItemRole:
        res = QVariant::fromValue<ContactObject*>(p->telegram->contact(key));
        break;
    }

    return res;
}

QHash<qint32, QByteArray> TelegramContactsModel::roleNames() const
{
    static QHash<qint32, QByteArray> *res = 0;
    if( res )
        return *res;

    res = new QHash<qint32, QByteArray>();
    res->insert( ItemRole, "item");
    return *res;
}

int TelegramContactsModel::count() const
{
    return p->contacts.count();
}

bool TelegramContactsModel::initializing() const
{
    return p->initializing;
}

void TelegramContactsModel::refresh()
{
    if( !p->telegram )
        return;

    contactsChanged();
    recheck();

    p->initializing = true;
    Q_EMIT initializingChanged();
}

void TelegramContactsModel::recheck()
{
    if( !p->telegram || !p->telegram->authLoggedIn() )
        return;
    Telegram *tg = p->telegram->telegram();
    if(tg)
        tg->contactsGetContacts();
}

void TelegramContactsModel::contactsChanged()
{
    if(!p->telegram)
        return;

    const QList<qint64> & contacts_unsort = p->telegram->contacts();
    QMap<QString,qint64> sort_map;
    Q_FOREACH( qint64 id, contacts_unsort )
    {
        UserObject *user = p->telegram->user(id);
        sort_map.insertMulti( user->firstName() + " " + user->lastName(), id );
    }

    const QList<qint64> & contacts = sort_map.values();

    beginResetModel();
    p->contacts.clear();
    endResetModel();

    for( int i=0 ; i<contacts.count() ; i++ )
    {
        const qint64 dId = contacts.at(i);
        if( p->contacts.contains(dId) )
            continue;

        beginInsertRows(QModelIndex(), i, i );
        p->contacts.insert( i, dId );
        endInsertRows();
    }

    p->initializing = false;
    Q_EMIT initializingChanged();
}

TelegramContactsModel::~TelegramContactsModel()
{
    delete p;
}
