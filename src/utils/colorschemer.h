/**
 * SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

class QAbstractItemModel;
class KColorSchemeManager;

class ColorSchemer : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)

public:
    static ColorSchemer *create(QQmlEngine *, QJSEngine *)
    {
        auto inst = &instance();
        QJSEngine::setObjectOwnership(inst, QJSEngine::ObjectOwnership::CppOwnership);
        return inst;
    }

    explicit ColorSchemer(QObject *parent = nullptr);

    static ColorSchemer &instance();

    QAbstractItemModel *model() const;
    Q_INVOKABLE void apply(int idx);
    Q_INVOKABLE void apply(const QString &name);
    Q_INVOKABLE int indexForScheme(const QString &name) const;
    Q_INVOKABLE QString nameForIndex(int index) const;

private:
    KColorSchemeManager *c;
};
