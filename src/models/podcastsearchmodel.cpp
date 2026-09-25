/**
 * SPDX-FileCopyrightText: 2021 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/podcastsearchmodel.h"

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
#include "models/errorlogmodel.h"

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
    case NameRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("title")].toString());
    case IdRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("id")].toInt());
    case UrlRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("url")].toString());
    case OriginalUrlRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("originalUrl")].toString());
    case LinkRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("link")].toString());
    case DescriptionRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("description")].toString());
    case AuthorRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("author")].toString());
    case OwnerNameRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("ownerName")].toString());
    case ImageRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("image")].toString());
    case ArtworkRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("artwork")].toString());
    case LastUpdatedRole:
        return QVariant::fromValue(
            QDateTime::fromSecsSinceEpoch(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastUpdateTime")].toInt()));
    case LastCrawlTimeRole:
        return QVariant::fromValue(
            QDateTime::fromSecsSinceEpoch(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastCrawlTime")].toInt()));
    case LastParseTimeRole:
        return QVariant::fromValue(
            QDateTime::fromSecsSinceEpoch(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastParseTime")].toInt()));
    case LastGoodHttpStatusTimeRole:
        return QVariant::fromValue(
            QDateTime::fromSecsSinceEpoch(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastGoodHttpStatusTime")].toInt()));
    case LastHttpStatusRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastHttpStatus")].toInt());
    case ContentTypeRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("contentType")].toString());
    case ItunesIdRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("itunesId")].toInt());
    case GeneratorRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("generator")].toString());
    case LanguageRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("language")].toString());
    case TypeRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("type")].toInt());
    case DeadRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("dead")].toInt());
    case CrawlErrorsRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("crawlErrors")].toInt());
    case ParseErrorsRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("parseErrors")].toInt());
    case CategoriesRole: {
        // TODO: Implement this function to add to the list of categories.
    }
    case LockedRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("locked")].toInt());
    case ImageUrlHashRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("imageUrlHash")].toInt());
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PodcastSearchModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {IdRole, "id"},
        {UrlRole, "url"},
        {OriginalUrlRole, "originalUrl"},
        {LinkRole, "link"},
        {DescriptionRole, "description"},
        {AuthorRole, "author"},
        {OwnerNameRole, "ownerName"},
        {ImageRole, "image"},
        {ArtworkRole, "artwork"},
        {LastUpdatedRole, "lastUpdated"},
        {LastCrawlTimeRole, "lastCrawlTime"},
        {LastParseTimeRole, "lastParseTime"},
        {LastGoodHttpStatusTimeRole, "lastGoodHttpStatusTime"},
        {LastHttpStatusRole, "lastHttpStatus"},
        {ContentTypeRole, "contentType"},
        {ItunesIdRole, "itunesId"},
        {GeneratorRole, "generator"},
        {LanguageRole, "language"},
        {TypeRole, "type"},
        {DeadRole, "dead"},
        {CrawlErrorsRole, "crawlErrors"},
        {ParseErrorsRole, "parseErrors"},
        {CategoriesRole, "categories"},
        {LockedRole, "locked"},
        {ImageUrlHashRole, "imageUrlHash"},
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
            m_data = QJsonDocument::fromJson(reply->readAll()).object();
            endResetModel();
        }
        reply->deleteLater();
    });
}
