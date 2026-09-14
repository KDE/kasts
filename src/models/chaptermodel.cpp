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

#include "database.h"
#include "objectslogging.h"
#include "queuemodel.h"
#include "utils/storagemanager.h"

ChapterModel::ChapterModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qCDebug(kastsObjects) << "ChapterModel object constructed";
}

ChapterModel::~ChapterModel()
{
    qCDebug(kastsObjects) << "ChapterModel object destructed";
}

QVariant ChapterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case TitleRole:
        return QVariant::fromValue(m_chapters.at(index.row()).title);
    case LinkRole:
        return QVariant::fromValue(m_chapters.at(index.row()).link);
    case ImageRole:
        if (!m_chapters.at(index.row()).image.isEmpty()) {
            return QVariant::fromValue(m_chapters.at(index.row()).image);
        } else if (!m_entryImage.isEmpty()) {
            return QVariant::fromValue(m_entryImage);
        } else {
            return QVariant::fromValue(m_feedImage);
        }
    case StartTimeRole:
        return QVariant::fromValue(m_chapters.at(index.row()).start);
    case DurationRole:
        if (m_chapters.size() > index.row() + 1) {
            return QVariant::fromValue(m_chapters.at(index.row() + 1).start - m_chapters.at(index.row()).start);
        } else {
            return QVariant::fromValue(m_duration / 1000 - m_chapters.at(index.row()).start);
        }
    case EntryuidRole:
        return QVariant::fromValue(m_entryuid);
    case QueueStatusRole:
        return QVariant::fromValue(QueueModel::instance().entryInQueue(m_entryuid));
    case EnclosureStatusRole:
        return QVariant::fromValue(m_enclosureStatus);

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
        {TitleRole, "title"},
        {LinkRole, "link"},
        {ImageRole, "image"},
        {StartTimeRole, "start"},
        {DurationRole, "duration"},
        {EntryuidRole, "entryuid"},
        {QueueStatusRole, "queueStatus"},
        {EnclosureStatusRole, "enclosureStatus"},
    };
}

qint64 ChapterModel::entryuid() const
{
    return m_entryuid;
}

void ChapterModel::setEntryuid(const qint64 entryuid)
{
    m_entryuid = entryuid;

    load();
    Q_EMIT entryuidChanged();
}

void ChapterModel::load()
{
    beginResetModel();
    m_chapters.clear();
    m_currentChapter = 0;

    if (m_entryuid > 0) {
        // First get the entry and feed data
        QSqlQuery query;
        query.prepare(
            QStringLiteral("SELECT * FROM Entries JOIN Enclosures ON Enclosures.entryuid= Entries.entryuid JOIN Feeds ON Feeds.feeduid=Entries.feeduid WHERE "
                           "Entries.entryuid=:entryuid AND (Enclosures.type LIKE '%audio%' OR type LIKE '%video%')"));
        query.bindValue(QStringLiteral(":entryuid"), m_entryuid);
        Database::instance().execute(query);
        if (query.next()) {
            m_entryId = query.value(QStringLiteral("Entries.id")).toString();
            m_entryTitle = query.value(QStringLiteral("Entries.title")).toString();
            m_entryImage = query.value(QStringLiteral("Entries.image")).toString();
            m_enclosureUrl = query.value(QStringLiteral("Enclosures.url")).toString();
            m_enclosureStatus = DataTypes::dbToStatus(query.value(QStringLiteral("Enclosures.downloaded")).toInt());
            m_feedDirName = query.value(QStringLiteral("Feeds.dirname")).toString();
            m_feedImage = query.value(QStringLiteral("Feeds.image")).toString();

            loadChaptersFromFile();
            if (m_chapters.isEmpty()) {
                loadFromDatabase();
            }
        }
    }

    endResetModel();
    Q_EMIT hasChaptersChanged();
}

void ChapterModel::loadFromDatabase()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Chapters WHERE entryuid=:entryuid ORDER BY start ASC;"));
    query.bindValue(QStringLiteral(":entryuid"), m_entryuid);
    Database::instance().execute(query);
    while (query.next()) {
        DataTypes::ChapterUpdateDetails chapter;
        chapter.title = query.value(QStringLiteral("title")).toString();
        chapter.link = query.value(QStringLiteral("link")).toString();
        chapter.image = query.value(QStringLiteral("image")).toString();
        chapter.start = query.value(QStringLiteral("start")).toInt();
        m_chapters << chapter;
    }
}

void ChapterModel::loadMPEGChapters()
{
    TagLib::MPEG::File f(StorageManager::enclosurePath(m_entryTitle, m_enclosureUrl, m_feedDirName).toStdString().data());

    if (!f.isValid() || !f.hasID3v2Tag()) {
        return;
    }
    for (const auto &frame : f.ID3v2Tag()->frameListMap()["CHAP"]) {
        DataTypes::ChapterUpdateDetails chapter;
        auto chapterFrame = dynamic_cast<TagLib::ID3v2::ChapterFrame *>(frame);

        const auto &apicList = chapterFrame->embeddedFrameListMap()["APIC"];
        QString imageName = QStringLiteral("%1,%2").arg(m_entryId).arg(chapterFrame->startTime());
        QString path = StorageManager::imagePath(imageName);
        chapter.image = QUrl::fromLocalFile(path).toString();
        if (!apicList.isEmpty()) {
            if (!QFileInfo::exists(path)) {
                QFile file(path);
                const auto apic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(apicList.front())->picture();
                if (file.open(QFile::WriteOnly)) {
                    file.write(QByteArray(apic.data(), apic.size()));
                    file.close();
                } else {
                    chapter.image = QString();
                }
            }
        } else {
            chapter.image = QString();
        }

        const auto frameListMap = chapterFrame->embeddedFrameListMap()["TIT2"];
        chapter.title = frameListMap.isEmpty() ? i18nc("@info", "Unnamed chapter")
                                               : QString::fromStdString(chapterFrame->embeddedFrameListMap()["TIT2"].front()->toString().to8Bit(true));
        chapter.start = chapterFrame->startTime() / 1000;
        auto originalChapter = std::find_if(m_chapters.begin(), m_chapters.end(), [chapter](auto it) {
            return chapter.start == it.start;
        });
        if (originalChapter != m_chapters.end()) {
            (*originalChapter).image = chapter.image;
        } else {
            m_chapters << chapter;
        }
    }
    std::sort(m_chapters.begin(), m_chapters.end(), [](const DataTypes::ChapterUpdateDetails a, const DataTypes::ChapterUpdateDetails b) {
        return a.start < b.start;
    });
}

bool ChapterModel::hasChapters() const
{
    return m_chapters.length() > 0;
}

void ChapterModel::loadChaptersFromFile()
{
    QString enclosurePath = StorageManager::enclosurePath(m_entryTitle, m_enclosureUrl, m_feedDirName);
    if (m_enclosureUrl.isEmpty() || m_enclosureStatus != DataTypes::EnclosureStatus::Downloaded || enclosurePath.isEmpty()) {
        return;
    }

    const auto mime = QMimeDatabase().mimeTypeForFile(enclosurePath).name();
    if (mime == QStringLiteral("audio/mpeg")) {
        loadMPEGChapters();
    } // TODO else...
}

QString ChapterModel::imageForPosition(const qint64 position) const
{
    for (int i = 0; i < m_chapters.size(); i++) {
        if (m_chapters[i].start < position / 1000 && (i == m_chapters.size() - 1 || m_chapters[i + 1].start > position / 1000)) {
            return m_chapters[i].image;
        }
    }
    return QStringLiteral("");
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
