/*
 * Copyright 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "searchhistorymodel.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

SearchHistory::SearchHistory(QObject *parent, const QString &searchTerm)
    : QObject(parent)
    , m_searchTerm(searchTerm)
{
}

SearchHistory::SearchHistory(const QJsonObject &obj)
    : m_searchTerm(obj["searchTerm"].toString())
{
}

SearchHistory::~SearchHistory()
{
}

QJsonObject SearchHistory::toJson() const
{
    QJsonObject obj;
    obj["searchTerm"] = m_searchTerm;
    return obj;
}

void SearchHistory::setSearchTerm(const QString &searchTerm)
{
    m_searchTerm = searchTerm;
    emit propertyChanged();
}

//////////////// MODEL ///////////////////////////

SearchHistoryModel::SearchHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_settings = new QSettings(parent);
    load();
}

SearchHistoryModel::~SearchHistoryModel()
{
    save();
    delete m_settings;

    qDeleteAll(m_searches);
}

void SearchHistoryModel::load()
{
    QJsonDocument doc = QJsonDocument::fromJson(m_settings->value(QStringLiteral("searches")).toString().toUtf8());

    const auto array = doc.array();
    std::transform(array.begin(), array.end(), std::back_inserter(m_searches), [](const QJsonValue &ser) {
        return new SearchHistory(ser.toObject());
    });
}

void SearchHistoryModel::save()
{
    QJsonArray arr;

    const auto searches = qAsConst(m_searches);
    std::transform(searches.begin(), searches.end(), std::back_inserter(arr), [](const SearchHistory *search) {
        return QJsonValue(search->toJson());
    });

    m_settings->setValue(QStringLiteral("searches"), QString(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

QHash<int, QByteArray> SearchHistoryModel::roleNames() const
{
    return {{Roles::SearchHistoryRole, "search"}};
}

QVariant SearchHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_searches.count() || index.row() < 0)
        return {};

    auto *search = m_searches.at(index.row());
    if (role == Roles::SearchHistoryRole)
        return QVariant::fromValue(search);

    return {};
}

int SearchHistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_searches.count();
}

void SearchHistoryModel::insertSearchResult(QString searchTerm)
{
    Q_EMIT beginInsertRows({}, 0, 0);
    m_searches.insert(0, new SearchHistory(this, searchTerm));
    Q_EMIT endInsertRows();

    save();
}

void SearchHistoryModel::deleteSearchResult(const int index)
{
    beginRemoveRows({}, index, index);
    m_searches.removeAt(index);
    endRemoveRows();

    save();
}
