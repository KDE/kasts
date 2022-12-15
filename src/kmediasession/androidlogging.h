/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QByteArray>
#include <QString>

#include <android/log.h>

const char *applicationName = "org.kde.kmediasession";
void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)

    QByteArray localMsg = msg.toLocal8Bit();
    // const char *file = context.file ? context.file : "";
    // const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        __android_log_write(ANDROID_LOG_DEBUG, applicationName, localMsg.constData());
        break;
    case QtInfoMsg:
        __android_log_write(ANDROID_LOG_INFO, applicationName, localMsg.constData());
        break;
    case QtWarningMsg:
        __android_log_write(ANDROID_LOG_WARN, applicationName, localMsg.constData());
        break;
    case QtCriticalMsg:
        __android_log_write(ANDROID_LOG_ERROR, applicationName, localMsg.constData());
        break;
    case QtFatalMsg:
    default:
        __android_log_write(ANDROID_LOG_FATAL, applicationName, localMsg.constData());
        abort();
    }
}
