// SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#pragma once

#include <QtQml/qqmlregistration.h>

#include "kmediasession.h"
#include "metadata.h"

struct KMediaSessionForeign {
    Q_GADGET
    QML_FOREIGN(KMediaSession)
    QML_NAMED_ELEMENT(KMediaSession)
};

struct MetaDataForeign {
    Q_GADGET
    QML_FOREIGN(MetaData)
    QML_NAMED_ELEMENT(MetaData)
};
