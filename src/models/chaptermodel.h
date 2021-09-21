/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KFormat>
#include <QAbstractListModel>

struct ChapterEntry {
    QString title;
    QString link;
    QString image;
    int start;
};

class ChapterModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString enclosureId READ enclosureId WRITE setEnclosureId NOTIFY enclosureIdChanged)

public:
    enum RoleNames {
        Title = Qt::UserRole,
        Link,
        Image,
        StartTime,
        FormattedStartTime,
    };

    explicit ChapterModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    void setEnclosureId(QString newEnclosureId);
    QString enclosureId() const;

Q_SIGNALS:
    void enclosureIdChanged();

private:
    void loadFromDatabase();

    QString m_enclosureId;
    QVector<ChapterEntry> m_chapters;
    KFormat m_kformat;
};
