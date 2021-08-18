/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "searchhistorymodel.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

const QString SEARCHHISTORY_CFG_GROUP = QStringLiteral("General"), SEARCHHISTORY_CFG_KEY = QStringLiteral("searches");

SearchHistory::SearchHistory(QObject *parent, const QString &searchTerm)
    : QObject(parent)
    , m_searchTerm(searchTerm)
{
}

SearchHistory::SearchHistory(const QJsonObject &obj)
    : m_searchTerm(obj[QStringLiteral("searchTerm")].toString())
{
}

SearchHistory::~SearchHistory()
{
}

QJsonObject SearchHistory::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("searchTerm")] = m_searchTerm;
    return obj;
}

void SearchHistory::setSearchTerm(const QString &searchTerm)
{
    m_searchTerm = searchTerm;
    emit propertyChanged();
}

SearchHistoryModel::SearchHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
}

SearchHistoryModel::~SearchHistoryModel()
{
    save();

    qDeleteAll(m_searches);
}

void SearchHistoryModel::load()
{
    auto config = KSharedConfig::openConfig();
    KConfigGroup group = config->group(SEARCHHISTORY_CFG_GROUP);
    QJsonDocument doc = QJsonDocument::fromJson(group.readEntry(SEARCHHISTORY_CFG_KEY, "{}").toUtf8());

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

    auto config = KSharedConfig::openConfig();
    KConfigGroup group = config->group(SEARCHHISTORY_CFG_GROUP);
    group.writeEntry(SEARCHHISTORY_CFG_KEY, QJsonDocument(arr).toJson(QJsonDocument::Compact));

    group.sync();
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

void SearchHistoryModel::deleteSearchHistory()
{
    beginResetModel();
    for(auto &search : m_searches) {
        delete search;
    }
    m_searches.clear();
    endResetModel();
    qDebug() << "Success! Search history cleared.";

    save();
}
