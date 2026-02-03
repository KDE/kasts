/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KFormat>
#include <QAbstractListModel>
#include <QQmlEngine>

#include <mpegfile.h>

#include "chapter.h"
#include "entry.h"

class ChapterModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qint64 entryuid READ entryuid WRITE setEntryuid NOTIFY entryuidChanged)
    Q_PROPERTY(Chapter *currentChapter READ currentChapter NOTIFY currentChapterChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)

public:
    enum RoleNames {
        TitleRole = Qt::DisplayRole,
        LinkRole = Qt::UserRole + 1,
        ImageRole,
        StartTimeRole,
        FormattedStartTimeRole,
        ChapterRole,
        DurationRole,
        EntryRole,
        EntryuidRole,
    };
    Q_ENUM(RoleNames);

    explicit ChapterModel(QObject *parent = nullptr);
    ~ChapterModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    void setEntryuid(const qint64 entryuid);
    qint64 entryuid() const;

    Chapter *currentChapter() const;

    void setDuration(int duration);
    int duration() const;

Q_SIGNALS:
    void entryuidChanged();
    void currentChapterChanged();
    void durationChanged();

private:
    void load();
    void loadFromDatabase();
    void loadChaptersFromFile();
    void loadMPEGChapters();

    qint64 m_entryuid;
    QPointer<Entry> m_entry = nullptr;
    QVector<Chapter *> m_chapters;
    KFormat m_kformat;
    int m_currentChapter = 0;
    int m_duration;
};
