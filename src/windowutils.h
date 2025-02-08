// SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#pragma once

#include <QObject>
#include <QQmlEngine>

class QWindow;

class WindowUtils : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isWidescreen READ isWidescreen NOTIFY isWidescreenChanged)

public:
    static WindowUtils *create(QQmlEngine *, QJSEngine *)
    {
        static WindowUtils _instance;
        return &_instance;
    }

    bool isWidescreen() const;

Q_SIGNALS:
    void isWidescreenChanged();

private:
    WindowUtils();
    QWindow *m_window = nullptr;
};
