/**
 * SPDX-FileCopyrightText: 2021 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/podcastindexsearchmodel.h"

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

#include <KLocalizedString>

#include "fetcher.h"
#include "models/errorlogmodel.h"
#include "objectslogging.h"

PodcastIndexSearchModel::PodcastIndexSearchModel(QObject *parent)
    : AbstractPodcastSearchModel(parent)
{
    qCDebug(kastsObjects) << "PodcastIndexSearchModel constructed" << this;
}

PodcastIndexSearchModel::~PodcastIndexSearchModel()
{
    qCDebug(kastsObjects) << "PodcastIndexSearchModel destructed" << this;
}

void PodcastIndexSearchModel::parseJsonResults(const QJsonObject &data)
{
    m_feeds.clear();
    for (const auto result : data[QStringLiteral("feeds")].toArray()) {
        DataTypes::FeedDetails feed;
        feed.name = result.toObject()[QStringLiteral("title")].toString();
        feed.url = result.toObject()[QStringLiteral("url")].toString();
        feed.image = result.toObject()[QStringLiteral("image")].toString();
        feed.link = result.toObject()[QStringLiteral("link")].toString();
        feed.description = result.toObject()[QStringLiteral("description")].toString();
        feed.authors = result.toObject()[QStringLiteral("author")].toString();
        feed.lastUpdated = result.toObject()[QStringLiteral("lastUpdateTime")].toString().toLongLong();
        m_feeds.append(feed);
    }
}

QString PodcastIndexSearchModel::name() const
{
    return i18nc("@label:chooser Choice of search providers", "Podcastindex");
}

QString PodcastIndexSearchModel::placeholderText() const
{
    return i18nc("@label:textbox Placeholder text for search field", "Search podcastindex.org…");
}

void PodcastIndexSearchModel::search(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        beginResetModel();
        m_feeds.clear();
        endResetModel();
    } else {
        QString safeText(text);

        // TODO: Make this more urlsafe
        safeText.replace(QLatin1Char(' '), QLatin1Char('+'));

        QString url = QStringLiteral("https://api.podcastindex.org/api/1.0/search/byterm?q=%1").arg(text);
        QNetworkRequest request((QUrl(url)));
        request.setRawHeader("X-Auth-Key", "BLVCJJSWUJGD3WJQSZ56");
        auto time = QDateTime::currentSecsSinceEpoch();
        request.setRawHeader("X-Auth-Date", QString::number(time).toLatin1());
        QString hashString = QStringLiteral(
                                 "BLVCJJSWUJGD3WJQSZ56"
                                 "vSPPqM8Tqh9Lr3xbEb77L4$f6kAdVw3v$9TwzGRH")
            + QString::number(time);
        auto hash = QCryptographicHash::hash(hashString.toLatin1(), QCryptographicHash::Sha1);
        request.setRawHeader("Authorization", hash.toHex());
        auto reply = Fetcher::instance().get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
            if (reply->error()) {
                ErrorLogModel::instance().monitorErrorMessages(ErrorLogModel::Type::DiscoverError, reply->errorString());
            } else {
                beginResetModel();
                parseJsonResults(QJsonDocument::fromJson(reply->readAll()).object());
                endResetModel();
            }
            reply->deleteLater();
        });
    }
}
