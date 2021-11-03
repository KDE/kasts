# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

gradle_add_aar(solidextras_aar BUILDFILE ${CMAKE_CURRENT_SOURCE_DIR}/build.gradle NAME KastsSolidExtras)
gradle_install_aar(solidextras_aar DESTINATION jar)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/KastsSolidExtras-android-dependencies.xml
    ${CMAKE_BINARY_DIR}/lib/KastsSolidExtras_${CMAKE_ANDROID_ARCH_ABI}-android-dependencies.xml
)
install(FILES KastsSolidExtras-android-dependencies.xml DESTINATION ${KDE_INSTALL_LIBDIR} RENAME KastsSolidExtras_${CMAKE_ANDROID_ARCH_ABI}-android-dependencies.xml)
