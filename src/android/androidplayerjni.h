/*
   SPDX-FileCopyrightText: 2025 (c) Pedro Nishiyama <nishiyama.v3@gmail.com>

   SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QJniEnvironment>
#include <QJniObject>
#include <QObject>
#include <QString>
#include <QVariant>

class AndroidPlayerJni : public QObject
{
    Q_OBJECT

public:
    ~AndroidPlayerJni() override;

    static AndroidPlayerJni &instance()
    {
        static AndroidPlayerJni _instance;
        return _instance;
    }

    static void propertyChanged(const QString &property, const QVariant &value);

    static void registerNativeMethods();

Q_SIGNALS:

    void Next();

    void Previous();

    void PlayPause();

    void Stop();

    void Seek(qlonglong offset);

private:
    explicit AndroidPlayerJni(QObject *parent = nullptr);

    static void next(JNIEnv *, jobject);

    static void previous(JNIEnv *, jobject);

    static void playPause(JNIEnv *, jobject);

    static void stop(JNIEnv *, jobject);

    static void seek(JNIEnv *, jobject, qlonglong offset);

    static QJniObject metadataToJObject(const QVariantMap &metadata);
};
