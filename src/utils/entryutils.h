/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QString>

#include "datatypes.h"

namespace EntryUtils
{
QString entryImage(const QString &entryImage,
                   const QString &feedImage,
                   const QString &enclosureUrl,
                   const DataTypes::EnclosureStatus enclosureStatus,
                   const QString &entryTitle,
                   const QString &feedDirname);
QString
cachedEmbeddedImage(const QString &enclosureUrl, const DataTypes::EnclosureStatus enclosureStatus, const QString &entryTitle, const QString &feedDirname);
bool hasEnclosure(DataTypes::EntryDetails entryDetails);
}
