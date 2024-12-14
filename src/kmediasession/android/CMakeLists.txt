# SPDX-FileCopyrightText: 2024 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

gradle_add_aar(kasts_aar BUILDFILE ${CMAKE_CURRENT_SOURCE_DIR}/build.gradle NAME KMediaSession)
gradle_install_aar(kasts_aar DESTINATION jar)

install(
    FILES KMediaSession-android-dependencies.xml
    DESTINATION ${KDE_INSTALL_LIBDIR}
    RENAME KMediaSession_${CMAKE_ANDROID_ARCH_ABI}-android-dependencies.xml
)
