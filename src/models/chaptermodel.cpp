/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/chaptermodel.h"

#include <QDebug>
#include <QObject>
#include <QSqlQuery>

#include "database.h"

ChapterModel::ChapterModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant ChapterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int row = index.row();

    switch (role) {
    case Title:
        return QVariant::fromValue(m_chapters.at(row).title);
    case Link:
        return QVariant::fromValue(m_chapters.at(row).link);
    case Image:
        return QVariant::fromValue(m_chapters.at(row).image);
    case StartTime:
        return QVariant::fromValue(m_chapters.at(row).start);
    case FormattedStartTime:
        return QVariant::fromValue(m_kformat.formatDuration(m_chapters.at(row).start * 1000));
    default:
        return QVariant();
    }
}

int ChapterModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_chapters.count();
}

QHash<int, QByteArray> ChapterModel::roleNames() const
{
    return {
        {Title, "title"},
        {Link, "link"},
        {Image, "image"},
        {StartTime, "start"},
        {FormattedStartTime, "formattedStart"},
    };
}

QString ChapterModel::enclosureId() const
{
    return m_enclosureId;
}

void ChapterModel::setEnclosureId(QString newEnclosureId)
{
    m_enclosureId = newEnclosureId;
    loadFromDatabase();
    Q_EMIT enclosureIdChanged();
}

void ChapterModel::loadFromDatabase()
{
    beginResetModel();

    m_chapters = {};
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Chapters WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), enclosureId());
    Database::instance().execute(query);
    while (query.next()) {
        ChapterEntry chapter{};
        chapter.title = query.value(QStringLiteral("title")).toString();
        chapter.link = query.value(QStringLiteral("link")).toString();
        chapter.image = query.value(QStringLiteral("image")).toString();
        chapter.start = query.value(QStringLiteral("start")).toInt();
        m_chapters << chapter;
    }

    endResetModel();
}
