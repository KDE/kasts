/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/itunessearchmodel.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

#include <KLocalizedString>

#include "fetcher.h"
#include "models/errorlogmodel.h"
#include "objectslogging.h"

ItunesSearchModel::ItunesSearchModel(QObject *parent)
    : AbstractPodcastSearchModel(parent)
{
    qCDebug(kastsObjects) << "ItunesSearchModel constructed" << this;
}

ItunesSearchModel::~ItunesSearchModel()
{
    qCDebug(kastsObjects) << "ItunesSearchModel destructed" << this;
}

void ItunesSearchModel::parseJsonResults(const QJsonObject &data)
{
    m_feeds.clear();
    for (const auto result : data[QStringLiteral("results")].toArray()) {
        DataTypes::FeedDetails feed;
        feed.name = result.toObject()[QStringLiteral("collectionName")].toString();
        feed.url = result.toObject()[QStringLiteral("feedUrl")].toString();
        feed.image = result.toObject()[QStringLiteral("artworkUrl100")].toString();
        feed.link = result.toObject()[QStringLiteral("collectionViewUrl")].toString();
        feed.description = result.toObject()[QStringLiteral("collectionName")].toString();
        feed.authors = result.toObject()[QStringLiteral("artistName")].toString();
        feed.lastUpdated = QDateTime::fromString(result.toObject()[QStringLiteral("releaseDate")].toString(), Qt::ISODate).toSecsSinceEpoch();
        m_feeds.append(feed);
    }
}

QString ItunesSearchModel::name() const
{
    return i18nc("@label:chooser Choice of search providers", "iTunes");
}

QString ItunesSearchModel::placeholderText() const
{
    return i18nc("@label:textbox Placeholder text for search field", "Search iTunes…");
}

void ItunesSearchModel::search(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        beginResetModel();
        m_feeds.clear();
        endResetModel();
    } else {
        QString safeText(text);

        // TODO: Make this more urlsafe
        safeText.replace(QLatin1Char(' '), QLatin1Char('+'));

        QString url = QStringLiteral("https://itunes.apple.com/search?media=podcast&term=%1").arg(text);
        QNetworkRequest request((QUrl(url)));

        QNetworkReply *reply = Fetcher::instance().get(request);
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
