/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KJob>
#include <QNetworkReply>

class EnclosureDownloadJob : public KJob
{

public:
    explicit EnclosureDownloadJob(const QString &url, const QString &filename, const QString &title, QObject *parent = nullptr);

    void start() override;
    bool doKill() override;

private:
    QString m_url;
    QString m_filename;
    QString m_title;
    QNetworkReply *m_reply = nullptr;

    void startDownload();
};
