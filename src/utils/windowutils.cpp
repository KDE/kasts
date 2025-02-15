// SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "windowutils.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QWindow>

WindowUtils::WindowUtils()
    : QObject(nullptr)
{
    m_window = QGuiApplication::allWindows()[0];
    connect(m_window, &QWindow::widthChanged, this, &WindowUtils::isWidescreenChanged);
    connect(m_window, &QWindow::heightChanged, this, &WindowUtils::isWidescreenChanged);
}

bool WindowUtils::isWidescreen() const
{
    return m_window->width() > m_window->height();
}

QQuickItem *WindowUtils::focusedWindowItem()
{
    const auto window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    if (window) {
        return window->contentItem();
    } else {
        return nullptr;
    }
}
