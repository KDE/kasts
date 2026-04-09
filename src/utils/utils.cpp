// SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "utils.h"

#include <QGuiApplication>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QWindow>

Utils::Utils()
    : QObject(nullptr)
{
    auto allWindows = QGuiApplication::allWindows();
    m_window = allWindows[0];
    connect(m_window, &QWindow::widthChanged, this, &Utils::isWidescreenChanged);
    connect(m_window, &QWindow::heightChanged, this, &Utils::isWidescreenChanged);
}

bool Utils::isWidescreen() const
{
    return m_window->width() > m_window->height();
}

bool Utils::qtAbove69() const
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 9, 0))
    return true;
#else
    return false;
#endif
}

QString Utils::styleName() const
{
    return QQuickStyle::name();
}

QQuickItem *Utils::focusedWindowItem()
{
    const auto window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    if (window) {
        return window->contentItem();
    } else {
        return nullptr;
    }
}
