/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 * SPDX-FileCopyrightText: 2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/chaptermodel.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QObject>
#include <QSqlQuery>

#include <attachedpictureframe.h>
#include <chapterframe.h>

#include "audiomanager.h"
#include "database.h"
#include "storagemanager.h"

ChapterModel::ChapterModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&AudioManager::instance(), &AudioManager::positionChanged, this, [this]() {
        if (!m_entry || m_entry != AudioManager::instance().entry() || m_chapters.isEmpty()) {
            return;
        }
        if (m_chapters[m_currentChapter].start > AudioManager::instance().position() / 1000
            || (m_currentChapter < m_chapters.size() - 1 && m_chapters[m_currentChapter + 1].start < AudioManager::instance().position() / 1000)) {
            for (int i = 0; i < m_chapters.size(); i++) {
                if (m_chapters[i].start < AudioManager::instance().position() / 1000
                    && (i == m_chapters.size() - 1 || m_chapters[i + 1].start > AudioManager::instance().position() / 1000)) {
                    m_currentChapter = i;
                    Q_EMIT currentChapterChanged();
                }
            }
        }
    });
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

Entry *ChapterModel::entry() const
{
    return m_entry;
}

void ChapterModel::setEntry(Entry *entry)
{
    if (entry) {
        m_entry = entry;
    } else {
        m_entry = nullptr;
    }
    load();
    Q_EMIT entryChanged();
}

void ChapterModel::load()
{
    beginResetModel();
    m_chapters = {};
    m_currentChapter = 0;
    if (m_entry) {
        loadFromDatabase();
        loadChaptersFromFile();
    }
    endResetModel();
    Q_EMIT currentChapterChanged();
}

void ChapterModel::loadFromDatabase()
{
    if (m_entry) {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT * FROM Chapters WHERE id=:id ORDER BY start ASC;"));
        query.bindValue(QStringLiteral(":id"), m_entry->id());
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
}

void ChapterModel::loadMPEGChapters(TagLib::MPEG::File &f)
{
    if (!f.hasID3v2Tag()) {
        return;
    }
    for (const auto &frame : f.ID3v2Tag()->frameListMap()["CHAP"]) {
        auto chapterFrame = dynamic_cast<TagLib::ID3v2::ChapterFrame *>(frame);

        ChapterEntry chapter{};
        const auto &apicList = chapterFrame->embeddedFrameListMap()["APIC"];
        auto hash = QString::fromLatin1(
            QCryptographicHash::hash(QStringLiteral("%1,%2").arg(m_entry->id(), chapterFrame->startTime()).toLatin1(), QCryptographicHash::Md5).toHex());
        auto path = QStringLiteral("%1/images/%2").arg(StorageManager::instance().storagePath(), hash);
        if (!apicList.isEmpty()) {
            if (!QFileInfo(path).exists()) {
                QFile file(path);
                const auto apic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(apicList.front())->picture();
                file.open(QFile::WriteOnly);
                file.write(QByteArray(apic.data(), apic.size()));
                file.close();
            }
            chapter.image = path;
        } else {
            chapter.image = QString();
        }
        chapter.title = QString::fromStdString(chapterFrame->embeddedFrameListMap()["TIT2"].front()->toString().to8Bit(true));
        chapter.link = QString();
        chapter.start = chapterFrame->startTime() / 1000;
        auto originalChapter = std::find_if(m_chapters.begin(), m_chapters.end(), [chapter](auto it) {
            return chapter.start == it.start;
        });
        if (originalChapter != m_chapters.end()) {
            originalChapter->image = chapter.image;
        } else {
            m_chapters << chapter;
        }
    }
    std::sort(m_chapters.begin(), m_chapters.end(), [](const ChapterEntry &a, const ChapterEntry &b) {
        return a.start < b.start;
    });
}

void ChapterModel::loadChaptersFromFile()
{
    if (!m_entry || !m_entry->hasEnclosure() || m_entry->enclosure()->path().isEmpty()) {
        return;
    }
    const auto mime = QMimeDatabase().mimeTypeForFile(m_entry->enclosure()->path()).name();
    if (mime == QStringLiteral("audio/mpeg")) {
        TagLib::MPEG::File f(m_entry->enclosure()->path().toLatin1().data());
        loadMPEGChapters(f);
    } // TODO else...
}

int ChapterModel::currentChapter() const
{
    for (int i = 0; i < m_chapters.size(); i++) {
        if (m_chapters[i].start < AudioManager::instance().position() / 1000
            && (i == m_chapters.size() - 1 || m_chapters[i + 1].start > AudioManager::instance().position() / 1000)) {
            return i;
        }
    }
    return 0;
}

QString ChapterModel::currentChapterImage() const
{
    if (m_chapters.size() <= m_currentChapter) {
        return {};
    }
    return m_chapters[m_currentChapter].image;
}
