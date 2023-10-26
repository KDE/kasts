/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "storagemovejob.h"
#include "storagemovejoblogging.h"

#include <QDir>
#include <QFile>
#include <QTimer>

#include <KLocalizedString>

StorageMoveJob::StorageMoveJob(const QString &from, const QString &to, QStringList &list, QObject *parent)
    : KJob(parent)
    , m_from(from)
    , m_to(to)
    , m_list(list)
{
}

void StorageMoveJob::start()
{
    QTimer::singleShot(0, this, &StorageMoveJob::moveFiles);
}

void StorageMoveJob::moveFiles()
{
    qCDebug(kastsStorageMoveJob) << "Begin moving" << m_list << "from" << m_from << "to" << m_to;

    bool success = true;

    QStringList fileList; // this list will contain all files that need to be moved

    for (QString item : m_list) {
        // make a list of files to be moved; path is relative to m_from
        if (QFileInfo(m_from + QStringLiteral("/") + item).isDir()) {
            // this item is a dir; now add all files in that subdir
            QStringList tempList = QDir(m_from + QStringLiteral("/") + item + QStringLiteral("/")).entryList(QDir::Files);
            for (QString file : tempList) {
                fileList += item + QStringLiteral("/") + file;
            }

            // if the item is a subdir, let's try to create it in the new location
            // if this fails, then the destination is not writeable, and the move
            // should be aborted
            success = QFileInfo().absoluteDir().mkpath(m_to + QStringLiteral("/") + item) && success;
        } else if (QFileInfo(m_from + QStringLiteral("/") + item).isFile()) {
            // this item is a file; simply add it to the list
            fileList += item;
        }
    }

    if (!success) {
        setError(2);
        setErrorText(i18n("Destination path not writable"));
        emitResult();
        return;
    }

    setTotalAmount(Files, fileList.size());
    setProcessedAmount(Files, 0);

    for (int i = 0; i < fileList.size(); i++) {
        // First check if we need to abort this job
        if (m_abort) {
            // Remove files that were already copied
            for (int j = 0; j < i; j++) {
                qCDebug(kastsStorageMoveJob) << "Removing file" << QDir(m_to).absoluteFilePath(fileList[j]);
                QFile(QDir(m_to).absoluteFilePath(fileList[j])).remove();
            }
            setError(1);
            setErrorText(i18n("Operation aborted by user"));
            emitResult();
            return;
        }

        // Now we can start copying
        QString fromPath = QDir(m_from).absoluteFilePath(fileList[i]);
        QString toPath = QDir(m_to).absoluteFilePath(fileList[i]);
        if (QFileInfo::exists(toPath) && (QFileInfo(fromPath).size() == QFileInfo(toPath).size())) {
            qCDebug(kastsStorageMoveJob) << "Identical file already exists in destination; skipping" << toPath;
        } else {
            qCDebug(kastsStorageMoveJob) << "Copy" << fromPath << "to" << toPath;
            success = QFile(fromPath).copy(toPath) && success;
        }
        if (!success)
            break;
        setProcessedAmount(Files, i + 1);
    }

    if (m_abort) {
        setError(1);
        setErrorText(i18n("Operation aborted by user"));
    } else if (success) {
        // now it's safe to delete all the files from the original location
        for (QString file : fileList) {
            QFile(QDir(m_from).absoluteFilePath(file)).remove();
            qCDebug(kastsStorageMoveJob) << "Removing file" << QDir(m_from).absoluteFilePath(file);
        }

        // delete the directories as well
        for (const QString &item : m_list) {
            if (!item.isEmpty() && QFileInfo(m_from + QStringLiteral("/") + item).isDir()) {
                QDir(m_from).rmdir(item);
            }
        }
    } else {
        setError(2);
        setErrorText(i18n("An error occurred while copying data"));
    }

    emitResult();
}

bool StorageMoveJob::doKill()
{
    m_abort = true;
    return true;
}
