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

PodcastIndexSearchModel::PodcastIndexSearchModel(QObject *parent)
    : AbstractPodcastSearchModel(parent)
{
}

QVariant PodcastIndexSearchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount(QModelIndex())) {
        // invalid index
        return QVariant();
    }

    switch (role) {
    case NameRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("title")].toString());
    case UrlRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("url")].toString());
    case ImageRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("image")].toString());
    case LinkRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("link")].toString());
    case DescriptionRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("description")].toString());
    case AuthorRole:
        return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("author")].toString());
    case LastUpdatedRole:
        return QVariant::fromValue(
            QDateTime::fromSecsSinceEpoch(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastUpdateTime")].toInt()));
    // INFO: these roles are disabled as we're not using them right now; they could still be useful in the future
    // case IdRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("id")].toInt());
    // case OriginalUrlRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("originalUrl")].toString());
    // case OwnerNameRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("ownerName")].toString());
    // case ArtworkRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("artwork")].toString());
    // case LastCrawlTimeRole:
    //     return QVariant::fromValue(
    //         QDateTime::fromSecsSinceEpoch(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastCrawlTime")].toInt()));
    // case LastParseTimeRole:
    //     return QVariant::fromValue(
    //         QDateTime::fromSecsSinceEpoch(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastParseTime")].toInt()));
    // case LastGoodHttpStatusTimeRole:
    //     return QVariant::fromValue(
    //         QDateTime::fromSecsSinceEpoch(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastGoodHttpStatusTime")].toInt()));
    // case LastHttpStatusRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("lastHttpStatus")].toInt());
    // case ContentTypeRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("contentType")].toString());
    // case ItunesIdRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("itunesId")].toInt());
    // case GeneratorRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("generator")].toString());
    // case LanguageRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("language")].toString());
    // case TypeRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("type")].toInt());
    // case DeadRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("dead")].toInt());
    // case CrawlErrorsRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("crawlErrors")].toInt());
    // case ParseErrorsRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("parseErrors")].toInt());
    // case CategoriesRole: {
    //     // TODO: Implement this function to add to the list of categories.
    // }
    // case LockedRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("locked")].toInt());
    // case ImageUrlHashRole:
    //     return QVariant::fromValue(m_data[QStringLiteral("feeds")].toArray()[index.row()].toObject()[QStringLiteral("imageUrlHash")].toInt());
    default:
        return QVariant();
    }
}

int PodcastIndexSearchModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (m_data.isEmpty()) {
        return 0;
    }
    return m_data[QStringLiteral("feeds")].toArray().size();
}

QString PodcastIndexSearchModel::placeholderText() const
{
    return i18nc("@label:textbox Placeholder text for search field", "Search podcastindex.org…");
}

void PodcastIndexSearchModel::search(const QString &text)
{
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
            m_data = QJsonDocument::fromJson(reply->readAll()).object();
            endResetModel();
        }
        reply->deleteLater();
    });
}
