/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>

#include "models/abstractpodcastsearchmodel.h"

class ItunesSearchModel : public AbstractPodcastSearchModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ItunesSearchModel(QObject *parent = nullptr);

    QString name() const override;
    QString placeholderText() const override;
    Q_INVOKABLE void search(const QString &text) override;

protected:
    void parseJsonResults(const QJsonObject &data);
};
