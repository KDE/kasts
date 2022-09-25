/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KFormat>
#include <QAbstractListModel>

#include <mpegfile.h>

#include "entry.h"

struct ChapterEntry {
    QString title;
    QString link;
    QString image;
    int start;
};

class ChapterModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(Entry *entry READ entry WRITE setEntry NOTIFY entryChanged)
    Q_PROPERTY(int currentChapter READ currentChapter NOTIFY currentChapterChanged)
    Q_PROPERTY(QString currentChapterImage READ currentChapterImage NOTIFY currentChapterChanged)

public:
    enum RoleNames {
        Title = Qt::DisplayRole,
        Link = Qt::UserRole + 1,
        Image,
        StartTime,
        FormattedStartTime,
    };
    Q_ENUM(RoleNames);

    explicit ChapterModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    void setEntry(Entry *entry);
    Entry *entry() const;

    int currentChapter() const;
    QString currentChapterImage() const;

Q_SIGNALS:
    void entryChanged();
    void currentChapterChanged();

private:
    void load();
    void loadFromDatabase();
    void loadChaptersFromFile();
    void loadMPEGChapters(TagLib::MPEG::File &f);

    Entry *m_entry = nullptr;
    QVector<ChapterEntry> m_chapters;
    KFormat m_kformat;
    int m_currentChapter = 0;
};
