/*
 * Copyright 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SEARCHHISTORYMODEL_H
#define SEARCHHISTORYMODEL_H

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QJsonObject>
#include <QObject>
#include <QSettings>

class SearchHistory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY propertyChanged)
public:
    explicit SearchHistory(QObject *parent = nullptr, const QString &searchTerm = {});
    explicit SearchHistory(const QJsonObject &obj);

    ~SearchHistory();

    QJsonObject toJson() const;

    QString searchTerm() const
    {
        return m_searchTerm;
    }

    void setSearchTerm(const QString &searchTerm);

private:
    QString m_searchTerm;

signals:
    void propertyChanged();
};

//MODEL

class SearchHistoryModel;
static SearchHistoryModel *s_searchHistoryModel = nullptr;

class SearchHistoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        SearchHistoryRole = Qt::UserRole
    };

    static SearchHistoryModel* instance()
    {
        if (!s_searchHistoryModel) {
            s_searchHistoryModel = new SearchHistoryModel(qApp);
        }
        return s_searchHistoryModel;
    }

    void load();
    void save();

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE void insertSearchResult(QString searchTerm);
    Q_INVOKABLE void deleteSearchResult(const int index);

private:
    explicit SearchHistoryModel(QObject *parent = nullptr);
    ~SearchHistoryModel();

    QSettings *m_settings;
    QList<SearchHistory *> m_searches;
};
#endif // SEARCHHISTORYMODEL_H

