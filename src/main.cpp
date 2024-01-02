/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QIcon>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickView>
#include <QString>
#include <QStringList>
#include <QSysInfo>
#include <QVariant>

#ifdef Q_OS_ANDROID
#include <QGuiApplication>
#else
#include <QApplication>
#endif

#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>

#ifdef Q_OS_ANDROID
#include "androidlogging.h"
#endif
#include "audiomanager.h"
#include "database.h"
#include "datamanager.h"
#include "entry.h"
#include "feed.h"
#include "fetcher.h"
#include "kasts-version.h"
#include "models/abstractepisodemodel.h"
#include "models/abstractepisodeproxymodel.h"
#include "models/chaptermodel.h"
#include "models/downloadmodel.h"
#include "models/entriesproxymodel.h"
#include "models/episodeproxymodel.h"
#include "models/errorlogmodel.h"
#include "models/feedsproxymodel.h"
#include "models/podcastsearchmodel.h"
#include "models/queuemodel.h"
#include "settingsmanager.h"
#include "sync/sync.h"
#include "sync/syncutils.h"
#include "utils/colorschemer.h"
#include "utils/networkconnectionmanager.h"
#include "utils/storagemanager.h"
#include "utils/systrayicon.h"

#ifdef Q_OS_WINDOWS
#include <windows.h>
#endif

#ifdef Q_OS_ANDROID
Q_DECL_EXPORT
#endif

int main(int argc, char *argv[])
{
    if (QSysInfo::currentCpuArchitecture().contains(QStringLiteral("arm")) && qEnvironmentVariableIsEmpty("QT_ENABLE_GLYPH_CACHE_WORKAROUND")) {
        qputenv("QT_ENABLE_GLYPH_CACHE_WORKAROUND", "1");
    }

#ifdef Q_OS_ANDROID
    QGuiApplication app(argc, argv);
    qInstallMessageHandler(myMessageHandler);
    QLoggingCategory::setFilterRules(QStringLiteral("org.kde.*=true"));
    QQuickStyle::setStyle(QStringLiteral("org.kde.breeze"));
#else
    QApplication app(argc, argv);
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
#endif

#ifdef Q_OS_WINDOWS
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }

    QApplication::setStyle(QStringLiteral("breeze"));
    auto font = app.font();
    font.setPointSize(10);
    app.setFont(font);
#endif

    QIcon::setFallbackSearchPaths(QIcon::fallbackSearchPaths() << QStringLiteral(":custom-icons"));

    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationName(QStringLiteral("Kasts"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    KLocalizedString::setApplicationDomain("kasts");

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("Podcast Application"));
    QCommandLineOption addFeedOption(QStringList() << QStringLiteral("a") << QStringLiteral("add"),
                                     i18n("Adds a new podcast to subscriptions."),
                                     i18n("Podcast URL"),
                                     QStringLiteral("none"));
    parser.addOption(addFeedOption);

    KAboutData about(QStringLiteral("kasts"),
                     i18n("Kasts"),
                     QStringLiteral(KASTS_VERSION_STRING),
                     i18n("Podcast Player"),
                     KAboutLicense::GPL,
                     i18n("© 2020–2023 KDE Community"));
    about.addAuthor(i18n("Tobias Fella"), QString(), QStringLiteral("tobias.fella@kde.org"), QStringLiteral("https://tobiasfella.de"));
    about.addAuthor(i18n("Bart De Vries"), QString(), QStringLiteral("bart@mogwai.be"));
    KAboutData::setApplicationData(about);

    about.setupCommandLine(&parser);
    parser.process(app);
    QString feedURL = parser.value(addFeedOption);
    Database::instance();
    if (feedURL != QStringLiteral("none")) {
        DataManager::instance().addFeed(feedURL);
    }
    about.processCommandLine(&parser);

    qmlRegisterSingletonInstance("org.kde.kasts.settings", 1, 0, "SettingsManager", SettingsManager::self());

    if (SettingsManager::self()->colorScheme().isEmpty()) {
        ColorSchemer::instance().apply(SettingsManager::self()->colorScheme());
    }

    // Workaround to don't get the app to silently quit when minimized to tray
    app.setQuitLockEnabled(false);

    // Make sure that settings are saved before the application exits
    QObject::connect(&app, &QCoreApplication::aboutToQuit, SettingsManager::self(), &SettingsManager::save);

    engine.loadFromModule("org.kde.kasts", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
