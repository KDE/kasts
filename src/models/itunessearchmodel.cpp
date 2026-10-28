/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/itunessearchmodel.h"

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

ItunesSearchModel::ItunesSearchModel(QObject *parent)
    : AbstractPodcastSearchModel(parent)
{
}

QVariant ItunesSearchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount(QModelIndex())) {
        // invalid index
        return QVariant();
    }

    switch (role) {
    case NameRole:
        return QVariant::fromValue(m_data[QStringLiteral("results")].toArray()[index.row()].toObject()[QStringLiteral("collectionName")].toString());
    case UrlRole:
        return QVariant::fromValue(m_data[QStringLiteral("results")].toArray()[index.row()].toObject()[QStringLiteral("feedUrl")].toString());
    case ImageRole:
        return QVariant::fromValue(m_data[QStringLiteral("results")].toArray()[index.row()].toObject()[QStringLiteral("artworkUrl100")].toString());
    case LinkRole:
        return QVariant::fromValue(m_data[QStringLiteral("results")].toArray()[index.row()].toObject()[QStringLiteral("collectionViewUrl")].toString());
    case DescriptionRole:
        return QVariant::fromValue(m_data[QStringLiteral("results")].toArray()[index.row()].toObject()[QStringLiteral("collectionName")].toString());
    case AuthorRole:
        return QVariant::fromValue(m_data[QStringLiteral("results")].toArray()[index.row()].toObject()[QStringLiteral("artistName")].toString());
    case LastUpdatedRole:
        return QVariant::fromValue(
            QDateTime::fromSecsSinceEpoch(m_data[QStringLiteral("results")].toArray()[index.row()].toObject()[QStringLiteral("releaseDate")].toInt()));
    default:
        return QVariant();
    }
}

int ItunesSearchModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (m_data.isEmpty()) {
        return 0;
    }
    return m_data[QStringLiteral("results")].toArray().size();
}

QString ItunesSearchModel::placeholderText() const
{
    return i18nc("@label:textbox Placeholder text for search field", "Search itunes…");
}

void ItunesSearchModel::search(const QString &text)
{
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
            m_data = QJsonDocument::fromJson(reply->readAll()).object();
            endResetModel();
        }
        reply->deleteLater();
    });
}
