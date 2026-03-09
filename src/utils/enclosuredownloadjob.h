/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkReply>
#include <QObject>
#include <QString>

#include <KJob>

class EnclosureDownloadJob : public KJob
{
    Q_OBJECT

public:
    enum Status {
        Queued,
        Downloading,
        Canceled,
    };
    Q_ENUM(Status)

    explicit EnclosureDownloadJob(const qint64 entryuid, const QString &url, const QString &filename, const QString &title, QObject *parent = nullptr);
    ~EnclosureDownloadJob();

    void start() override;
    bool doKill() override;
    Status status() const;

Q_SIGNALS:
    void statusChanged(EnclosureDownloadJob::Status status);

private:
    void startDownload();
    QNetworkReply *getNetworkReply(const QString &url, const QString &filePath) const;

    qint64 m_entryuid;
    QString m_url;
    QString m_filename;
    QString m_title;
    QNetworkReply *m_reply = nullptr;
    Status m_status = Queued;
};
