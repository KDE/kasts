/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "utils/entryutils.h"
#include "utils/entryutilslogging.h"

#include <QFileInfo>
#include <QImage>
#include <QMimeDatabase>
#include <QString>
#include <QUrl>

#include <attachedpictureframe.h>
#include <fileref.h>
#include <id3v2frame.h>
#include <id3v2tag.h>
#include <mpegfile.h>

#include "datamanager.h"
#include "storagemanager.h"

EntryUtils::EntryUtils(QObject *parent)
    : QObject(parent)
{
}

QString EntryUtils::entryImage(const QString &entryImage,
                               const QString &feedImage,
                               const QString &enclosureUrl,
                               const DataTypes::EnclosureStatus enclosureStatus,
                               const QString &entryTitle,
                               const QString &feedDirname)
{
    QString cachedEmbeddedImage = EntryUtils::cachedEmbeddedImage(enclosureUrl, enclosureStatus, entryTitle, feedDirname);
    if (!cachedEmbeddedImage.isEmpty()) {
        // use embedded image if available
        return cachedEmbeddedImage;
    } else if (!entryImage.isEmpty()) {
        return entryImage;
    } else {
        // else fall back to feed image
        return feedImage;
    }
}

QString EntryUtils::cachedEmbeddedImage(const QString &enclosureUrl,
                                        const DataTypes::EnclosureStatus enclosureStatus,
                                        const QString &entryTitle,
                                        const QString &feedDirname)
{
    QString path = StorageManager::enclosurePath(entryTitle, enclosureUrl, feedDirname);

    if (enclosureStatus != DataTypes::EnclosureStatus::Downloaded || path.isEmpty()) {
        return QLatin1String("");
    }

    // if image is already cached, then return the path
    QString cachedpath = StorageManager::imagePath(enclosureUrl);
    if (QFileInfo::exists(cachedpath)) {
        if (QFileInfo(cachedpath).size() != 0) {
            return QUrl::fromLocalFile(cachedpath).toString();
        }
    }
    const auto mime = QMimeDatabase().mimeTypeForFile(path).name();
    if (mime != QStringLiteral("audio/mpeg")) {
        return QLatin1String("");
    }

    TagLib::MPEG::File f(path.toStdString().data());
    if (!f.isValid() || !f.hasID3v2Tag()) {
        return QLatin1String("");
    }

    bool imageFound = false;
    for (const auto &frame : f.ID3v2Tag()->frameListMap()["APIC"]) {
        auto pictureFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(frame);
        QByteArray data(pictureFrame->picture().data(), pictureFrame->picture().size());
        if (!data.isEmpty() && QImage().loadFromData(data)) {
            QFile file(cachedpath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
                imageFound = true;
            }
        }
    }

    if (imageFound) {
        return QUrl::fromLocalFile(cachedpath).toString();
    } else {
        return QLatin1String("");
    }
}

qint64 EntryUtils::checkSizeOnDisk(const qint64 entryuid, const QString &filename, const qint64 size, bool updateStatus)
{
    // In principle the database contains this status, we check anyway in case
    // something changed on disk
    qint64 sizeOnDisk = 0;

    QFile file(filename);
    if (file.exists()) {
        if (file.size() == size && file.size() > 0) {
            // file is on disk and has correct size, write to database if it
            // wasn't already registered so
            // this should, in principle, never happen unless the db was deleted
            if (updateStatus)
                DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloaded}),
                                                                 QList<qint64>({entryuid}));
        } else if (file.size() > 0) {
            // file was downloaded, but there is a size mismatch
            // set to PartiallyDownloaded such that download can be resumed
            if (updateStatus)
                DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::PartiallyDownloaded}),
                                                                 QList<qint64>({entryuid}));
        } else {
            // file is empty
            if (updateStatus)
                DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloadable}),
                                                                 QList<qint64>({entryuid}));
        }
        sizeOnDisk = file.size();
    } else {
        // file does not exist
        if (updateStatus)
            DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloadable}),
                                                             QList<qint64>({entryuid}));
        sizeOnDisk = 0;
    }
    return sizeOnDisk;
}

QString EntryUtils::adjustedContent(const int width, const int fontSize, const QString &content, const QString &link)
{
    static QRegularExpression imgRegex(QStringLiteral("<img ((?!width=\"[0-9]+(px)?\").)*(width=\"([0-9]+)(px)?\")?[^>]*>"));
    static QRegularExpression imgHeightRegex(QStringLiteral("height=\"([0-9]+)(px)?\""));

    QString ret(content);

    QRegularExpressionMatchIterator i = imgRegex.globalMatch(ret);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        QString imgTag(match.captured());
        if (imgTag.contains(QStringLiteral("wp-smiley")))
            imgTag.insert(4, QStringLiteral(" width=\"%1\"").arg(fontSize));

        QString widthParameter = match.captured(4);

        if (widthParameter.length() != 0) {
            if (widthParameter.toInt() > width) {
                imgTag.replace(match.captured(3), QStringLiteral("width=\"%1\"").arg(width));
                imgTag.replace(imgHeightRegex, QString());
            }
        }
        ret.replace(match.captured(), imgTag);
    }

    ret.replace(QStringLiteral("<img"), QStringLiteral("<br /> <img"));

    // check image sources with no basepath; these will make qt6 crash because
    // it will try to retrieve them as if they were local resources
    static QRegularExpression imgSrcUrlRegex(QStringLiteral("<img .*?src=\"([\\S]+)\".*?>"));

    i = imgSrcUrlRegex.globalMatch(ret);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        QUrl imgSrcUrl = QUrl(match.captured(1));
        QUrl linkUrl = QUrl(link);

        if (imgSrcUrl.host().isEmpty()) {
            if (!linkUrl.authority().isEmpty()) {
                // if no host is set, then transplant the scheme and authority from
                // the episode link url; it's a wild guess, but it will keep qt from
                // trying to get it from the local resources and crashing
                QString imgTag(match.captured());
                QString linkTruncatedPath = linkUrl.path().left(linkUrl.path().lastIndexOf(QStringLiteral("/")) + 1);
                QString imgPath = imgSrcUrl.path();
                if (imgPath[0] == QStringLiteral("/")) {
                    imgPath.removeAt(0);
                }

                qCDebug(kastsEntryUtils) << imgSrcUrl << linkUrl << imgTag << linkTruncatedPath << imgPath;

                imgSrcUrl.setPath(linkTruncatedPath + imgPath);
                imgSrcUrl.setAuthority(linkUrl.authority());
                imgSrcUrl.setScheme(linkUrl.scheme());
                qCDebug(kastsEntryUtils) << imgSrcUrl;

                imgTag.replace(match.captured(1), imgSrcUrl.toString());
                ret.replace(match.captured(), imgTag);

                qCDebug(kastsEntryUtils) << imgTag;
            } else {
                // if there's no link to extract the basepath from, we simply
                // remove the image altogether
                ret.replace(match.captured(), QStringLiteral(""));
                qCDebug(kastsEntryUtils) << "Removed image with no host/basepath" << match.captured();
            }
        }
    }

    // Replace strings that look like timestamps into clickable links with scheme
    // "timestamp://".  We will pick these up in the GUI to work like chapter marks

    static QRegularExpression dateRegex(QStringLiteral("\\d{1,2}(:\\d{2})+"));

    i = dateRegex.globalMatch(ret);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString timeStamp(match.captured());
        QStringList timeFragments(timeStamp.split(QStringLiteral(":")));
        int timeUnit = 1;
        qint64 time = 0;
        for (QList<QString>::const_reverse_iterator iter = timeFragments.crbegin(); iter != timeFragments.crend(); iter++) {
            time += (*iter).toInt() * 1000 * timeUnit;
            timeUnit *= 60;
        }
        timeStamp = QStringLiteral("<a href=\"timestamp://%1\">%2</a>").arg(time).arg(timeStamp);
        ret.replace(match.captured(), timeStamp);
    }

    return ret;
}

QString EntryUtils::baseUrl(const QString &link)
{
    return QUrl(link).adjusted(QUrl::RemovePath).toString();
}
