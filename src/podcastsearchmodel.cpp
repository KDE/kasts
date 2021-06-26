/**
 * SPDX-FileCopyrightText: 2021 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "podcastsearchmodel.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QVariant>

#include "fetcher.h"

PodcastSearchModel::PodcastSearchModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant PodcastSearchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount(QModelIndex())) {
        // invalid index
        return QVariant::fromValue(QStringLiteral("DEADBEEF"));
    }
    if (role == Title) {
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("title")].toString();
    }
    return QVariant();
}

QHash<int, QByteArray> PodcastSearchModel::roleNames() const
{
    return {{Title, "title"}};
}

int PodcastSearchModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (m_data.isEmpty()) {
        return 0;
    }
    return m_data[QStringLiteral("feeds")].toArray().size();
}

void PodcastSearchModel::search(const QString &text)
{
    QString safeText(text);
    // TODO: Make this more urlsafe
    safeText.replace(QLatin1Char(' '), QLatin1Char('+'));
    QNetworkRequest request(QUrl(QStringLiteral("https://api.podcastindex.org/api/1.0/search/byterm?q=%1").arg(text)));
    request.setRawHeader("X-Auth-Key", "BLVCJJSWUJGD3WJQSZ56");
    auto time = QDateTime::currentDateTime().toSecsSinceEpoch();
    request.setRawHeader("X-Auth-Date", QString::number(time).toLatin1());
    QString hashString = QStringLiteral(
                             "BLVCJJSWUJGD3WJQSZ56"
                             "vSPPqM8Tqh9Lr3xbEb77L4$f6kAdVw3v$9TwzGRH")
        + QString::number(time);
    auto hash = QCryptographicHash::hash(hashString.toLatin1(), QCryptographicHash::Sha1);
    request.setRawHeader("Authorization", hash.toHex());
    auto reply = Fetcher::instance().get(request);
    // TODO: error handling
    connect(reply, &QNetworkReply::finished, this, [=]() {
        beginResetModel();
        m_data = QJsonDocument::fromJson(reply->readAll()).object();
        endResetModel();
    });
}
