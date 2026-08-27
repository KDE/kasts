/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "utils/entryutils.h"

#include <QFileInfo>
#include <QImage>
#include <QMimeDatabase>
#include <QString>

#include <attachedpictureframe.h>
#include <fileref.h>
#include <id3v2frame.h>
#include <id3v2tag.h>
#include <mpegfile.h>

#include "storagemanager.h"

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
    // if image is already cached, then return the path
    QString cachedpath = StorageManager::imagePath(enclosureUrl);
    if (QFileInfo::exists(cachedpath)) {
        if (QFileInfo(cachedpath).size() != 0) {
            return QUrl::fromLocalFile(cachedpath).toString();
        }
    }
    QString path = StorageManager::enclosurePath(entryTitle, enclosureUrl, feedDirname);

    if (enclosureStatus != DataTypes::EnclosureStatus::Downloaded || path.isEmpty()) {
        return QLatin1String("");
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
        return cachedpath;
    } else {
        return QLatin1String("");
    }
}

bool hasEnclosure(DataTypes::EntryDetails entryDetails)
{
    return !entryDetails.enclosures.isEmpty();
}
