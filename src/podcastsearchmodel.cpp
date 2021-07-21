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
    switch (role) {
    case Id:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("id")].toInt();
    case Title:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("title")].toString();
    case Url:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("url")].toString();
    case OriginalUrl:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("originalUrl")].toString();
    case Link:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("link")].toString();
    case Description:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("description")].toString();
    case Author:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("author")].toString();
    case OwnerName:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("ownerName")].toString();
    case Image:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("image")].toString();
    case Artwork:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("artwork")].toString();
    case LastUpdateTime:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastUpdateTime")].toInt();
    case LastCrawlTime:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastCrawlTime")].toInt();
    case LastParseTime:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastParseTime")].toInt();
    case LastGoodHttpStatusTime:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastGoodHttpStatusTime")].toInt();
    case LastHttpStatus:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastHttpStatus")].toInt();
    case ContentType:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("contentType")].toString();
    case ItunesId:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("itunesId")].toInt();
    case Generator:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("generator")].toString();
    case Language:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("language")].toString();
    case Type:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("type")].toInt();
    case Dead:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("dead")].toInt();
    case CrawlErrors:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("crawlErrors")].toInt();
    case ParseErrors:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("parseErrors")].toInt();
    case Categories: {
        // TODO: Implement this function to add to the list of categories.
    }
    case Locked:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("locked")].toInt();
    case ImageUrlHash:
        return m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("imageUrlHash")].toInt();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PodcastSearchModel::roleNames() const
{
    return {
        {Id, "id"},
        {Title, "title"},
        {Url, "url"},
        {OriginalUrl, "originalUrl"},
        {Link, "link"},
        {Description, "description"},
        {Author, "author"},
        {OwnerName, "ownerName"},
        {Image, "image"},
        {Artwork, "artwork"},
        {LastUpdateTime, "lastUpdateTime"},
        {LastCrawlTime, "lastCrawlTime"},
        {LastParseTime, "lastParseTime"},
        {LastGoodHttpStatusTime, "lastGoodHttpStatusTime"},
        {LastHttpStatus, "lastHttpStatus"},
        {ContentType, "contentType"},
        {ItunesId, "itunesId"},
        {Generator, "generator"},
        {Language, "language"},
        {Type, "type"},
        {Dead, "dead"},
        {CrawlErrors, "crawlErrors"},
        {ParseErrors, "parseErrors"},
        {Categories, "categories"},
        {Locked, "locked"},
        {ImageUrlHash, "imageUrlHash"},
    };
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
    QString url = QStringLiteral("https://api.podcastindex.org/api/1.0/search/byterm?q=%1").arg(text);
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
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if(reply->error())
        {
            ErrorLogModel::instance().monitorErrorMessages(Error::Type::DiscoverError, url, QString(), reply->error(), reply->errorString(), url);
        }
        else {
            beginResetModel();
            m_data = QJsonDocument::fromJson(reply->readAll()).object();
            endResetModel();
        }
    });
}
