/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/chaptermodel.h"

#include <QDebug>
#include <QMimeDatabase>
#include <QObject>
#include <QSqlQuery>

#include <chapterframe.h>

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
    load();
    Q_EMIT enclosureIdChanged();
}

void ChapterModel::load()
{
    beginResetModel();

    m_chapters = {};
    loadFromDatabase();
    if (m_chapters.isEmpty()) {
        loadChaptersFromFile();
    }
    endResetModel();
}

void ChapterModel::loadFromDatabase()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Chapters WHERE id=:id ORDER BY start ASC;"));
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
}

void ChapterModel::loadMPEGChapters(TagLib::MPEG::File &f)
{
    if (!f.hasID3v2Tag()) {
        return;
    }
    for (const auto &frame : f.ID3v2Tag()->frameListMap()["CHAP"]) {
        auto chapterFrame = dynamic_cast<TagLib::ID3v2::ChapterFrame *>(frame);

        ChapterEntry chapter{};
        chapter.title = QString::fromStdString(chapterFrame->embeddedFrameListMap()["TIT2"].front()->toString().to8Bit(true));
        chapter.link = QString();
        chapter.image = QString();
        chapter.start = chapterFrame->startTime() / 1000;
        m_chapters << chapter;
    }
    std::sort(m_chapters.begin(), m_chapters.end(), [](const ChapterEntry &a, const ChapterEntry &b) {
        return a.start < b.start;
    });
}

void ChapterModel::loadChaptersFromFile()
{
    if (m_enclosurePath.isEmpty()) {
        return;
    }
    const auto mime = QMimeDatabase().mimeTypeForFile(m_enclosurePath).name();
    if (mime == QStringLiteral("audio/mpeg")) {
        TagLib::MPEG::File f(m_enclosurePath.toLatin1().data());
        loadMPEGChapters(f);
    } // TODO else...
}

void ChapterModel::setEnclosurePath(const QString &enclosurePath)
{
    m_enclosurePath = enclosurePath;
    Q_EMIT enclosureIdChanged();
    load();
}

QString ChapterModel::enclosurePath() const
{
    return m_enclosurePath;
}
