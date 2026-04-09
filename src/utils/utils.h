// SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QString>

class QWindow;

class Utils : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isWidescreen READ isWidescreen NOTIFY isWidescreenChanged)
    Q_PROPERTY(bool qtAbove69 READ qtAbove69 CONSTANT)
    Q_PROPERTY(QString styleName READ styleName CONSTANT)

public:
    static Utils *create(QQmlEngine *, QJSEngine *)
    {
        static Utils _instance;
        QQmlEngine::setObjectOwnership(&_instance, QQmlEngine::CppOwnership);
        return &_instance;
    }

    bool isWidescreen() const;
    bool qtAbove69() const;
    QString styleName() const;
    Q_INVOKABLE QQuickItem *focusedWindowItem();

Q_SIGNALS:
    void isWidescreenChanged();

private:
    Utils();
    QWindow *m_window = nullptr;
};
