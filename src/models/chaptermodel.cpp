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

#include <KLocalizedString>

#include <attachedpictureframe.h>
#include <chapterframe.h>

#include "audiomanager.h"
#include "database.h"
#include "utils/storagemanager.h"

ChapterModel::ChapterModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&AudioManager::instance(), &AudioManager::positionChanged, this, [this]() {
        if (!m_entry || m_entry != AudioManager::instance().entry() || m_chapters.isEmpty()) {
            return;
        }
        if (m_chapters[m_currentChapter]
            && (m_chapters[m_currentChapter]->start() > AudioManager::instance().position() / 1000
                || (m_currentChapter < m_chapters.size() - 1 && m_chapters[m_currentChapter + 1]
                    && m_chapters[m_currentChapter + 1]->start() < AudioManager::instance().position() / 1000))) {
            for (int i = 0; i < m_chapters.size(); i++) {
                if (m_chapters[i]->start() < AudioManager::instance().position() / 1000
                    && (i == m_chapters.size() - 1 || m_chapters[i + 1]->start() > AudioManager::instance().position() / 1000)) {
                    m_currentChapter = i;
                    Q_EMIT currentChapterChanged();
                }
            }
        }
    });
}

ChapterModel::~ChapterModel()
{
    qDeleteAll(m_chapters.begin(), m_chapters.end());
}

QVariant ChapterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int row = index.row();
    if (m_chapters.at(row)) {
        switch (role) {
        case TitleRole:
            return QVariant::fromValue(m_chapters.at(row)->title());
        case LinkRole:
            return QVariant::fromValue(m_chapters.at(row)->link());
        case ImageRole:
            return QVariant::fromValue(m_chapters.at(row)->image());
        case StartTimeRole:
            return QVariant::fromValue(m_chapters.at(row)->start());
        case FormattedStartTimeRole:
            return QVariant::fromValue(m_kformat.formatDuration(m_chapters.at(row)->start() * 1000));
        case ChapterRole:
            return QVariant::fromValue(m_chapters.at(row));
        case DurationRole:
            if (m_chapters.size() > row + 1) {
                return QVariant::fromValue(m_chapters.at(row + 1)->start() - m_chapters.at(row)->start());
            } else {
                return QVariant::fromValue(m_duration / 1000 - m_chapters.at(row)->start());
            }

        default:
            return QVariant();
        }
    } else {
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
        {TitleRole, "title"},
        {LinkRole, "link"},
        {ImageRole, "image"},
        {StartTimeRole, "start"},
        {FormattedStartTimeRole, "formattedStart"},
        {ChapterRole, "chapter"},
        {DurationRole, "duration"},
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
        qDeleteAll(m_chapters.begin(), m_chapters.end());
        m_chapters.clear();
        m_entry = nullptr;
    }
    load();
    Q_EMIT entryChanged();
}

void ChapterModel::load()
{
    beginResetModel();
    qDeleteAll(m_chapters.begin(), m_chapters.end());
    m_chapters = {};
    m_currentChapter = 0;
    if (m_entry) {
        loadChaptersFromFile();
        if (m_chapters.isEmpty()) {
            loadFromDatabase();
        }
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
            Chapter *chapter = new Chapter(m_entry,
                                           query.value(QStringLiteral("title")).toString(),
                                           query.value(QStringLiteral("link")).toString(),
                                           query.value(QStringLiteral("image")).toString(),
                                           query.value(QStringLiteral("start")).toInt(),
                                           this);
            m_chapters << chapter;
        }
    }
}

void ChapterModel::loadMPEGChapters()
{
    TagLib::MPEG::File f(m_entry->enclosure()->path().toStdString().data());

    if (!f.isValid() || !f.hasID3v2Tag()) {
        return;
    }
    for (const auto &frame : f.ID3v2Tag()->frameListMap()["CHAP"]) {
        auto chapterFrame = dynamic_cast<TagLib::ID3v2::ChapterFrame *>(frame);

        const auto &apicList = chapterFrame->embeddedFrameListMap()["APIC"];
        QString image = QStringLiteral("%1,%2").arg(m_entry->id()).arg(chapterFrame->startTime());
        // TODO: get hashed filename from a method in Fetcher
        auto hash = QString::fromLatin1(QCryptographicHash::hash(image.toLatin1(), QCryptographicHash::Md5).toHex());
        auto path = QStringLiteral("%1/images/%2").arg(StorageManager::instance().storagePath(), hash);
        if (!apicList.isEmpty()) {
            if (!QFileInfo::exists(path)) {
                QFile file(path);
                const auto apic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(apicList.front())->picture();
                if (file.open(QFile::WriteOnly)) {
                    file.write(QByteArray(apic.data(), apic.size()));
                    file.close();
                } else {
                    image = QString();
                }
            }
        } else {
            image = QString();
        }
        const auto frameListMap = chapterFrame->embeddedFrameListMap()["TIT2"];
        QString title = frameListMap.isEmpty() ? i18nc("@info", "Unnamed chapter")
                                               : QString::fromStdString(chapterFrame->embeddedFrameListMap()["TIT2"].front()->toString().to8Bit(true));
        int start = chapterFrame->startTime() / 1000;
        Chapter *chapter = new Chapter(m_entry, title, QString(), image, start, this);
        auto originalChapter = std::find_if(m_chapters.begin(), m_chapters.end(), [chapter](auto it) {
            return chapter->start() == it->start();
        });
        if (originalChapter != m_chapters.end()) {
            (*originalChapter)->image() = chapter->image();
        } else {
            m_chapters << chapter;
        }
    }
    std::sort(m_chapters.begin(), m_chapters.end(), [](const Chapter *a, const Chapter *b) {
        return a->start() < b->start();
    });
}

void ChapterModel::loadChaptersFromFile()
{
    if (!m_entry || !m_entry->hasEnclosure() || m_entry->enclosure()->status() != Enclosure::Status::Downloaded || m_entry->enclosure()->path().isEmpty()) {
        return;
    }

    const auto mime = QMimeDatabase().mimeTypeForFile(m_entry->enclosure()->path()).name();
    if (mime == QStringLiteral("audio/mpeg")) {
        loadMPEGChapters();
    } // TODO else...
}

Chapter *ChapterModel::currentChapter() const
{
    for (int i = 0; i < m_chapters.size(); i++) {
        if (m_chapters[i] && m_chapters[i]->start() < AudioManager::instance().position() / 1000
            && (i == m_chapters.size() - 1 || m_chapters[i + 1]->start() > AudioManager::instance().position() / 1000)) {
            return m_chapters[i];
        }
    }
    return nullptr;
}

void ChapterModel::setDuration(int duration)
{
    m_duration = duration;
    Q_EMIT durationChanged();
}

int ChapterModel::duration() const
{
    return m_duration;
}
