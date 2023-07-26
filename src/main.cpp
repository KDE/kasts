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
#include "author.h"
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
#include "models/queueproxymodel.h"
#include "networkconnectionmanager.h"
#include "settingsmanager.h"
#include "storagemanager.h"
#include "sync/sync.h"
#include "sync/syncutils.h"
#include "systrayicon.h"

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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

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
    about.setProgramLogo(QVariant(QIcon(QStringLiteral(":/logo.svg"))));
    KAboutData::setApplicationData(about);

    about.setupCommandLine(&parser);
    parser.process(app);
    QString feedURL = parser.value(addFeedOption);
    if (feedURL != QStringLiteral("none")) {
        Database::instance();
        DataManager::instance().addFeed(feedURL);
    }
    about.processCommandLine(&parser);

    qmlRegisterType<FeedsProxyModel>("org.kde.kasts", 1, 0, "FeedsProxyModel");
    qmlRegisterType<QueueProxyModel>("org.kde.kasts", 1, 0, "QueueProxyModel");
    qmlRegisterType<EpisodeProxyModel>("org.kde.kasts", 1, 0, "EpisodeProxyModel");
    qmlRegisterType<PodcastSearchModel>("org.kde.kasts", 1, 0, "PodcastSearchModel");
    qmlRegisterType<ChapterModel>("org.kde.kasts", 1, 0, "ChapterModel");

    qmlRegisterUncreatableType<AbstractEpisodeProxyModel>("org.kde.kasts", 1, 0, "AbstractEpisodeProxyModel", QStringLiteral("Only for enums"));
    qmlRegisterUncreatableType<EntriesProxyModel>("org.kde.kasts", 1, 0, "EntriesProxyModel", QStringLiteral("Get from Feed"));
    qmlRegisterUncreatableType<Enclosure>("org.kde.kasts", 1, 0, "Enclosure", QStringLiteral("Only for enums"));
    qmlRegisterUncreatableType<AbstractEpisodeModel>("org.kde.kasts", 1, 0, "AbstractEpisodeModel", QStringLiteral("Only for enums"));
    qmlRegisterUncreatableType<FeedsModel>("org.kde.kasts", 1, 0, "FeedsModel", QStringLiteral("Only for enums"));

    qmlRegisterSingletonType("org.kde.kasts", 1, 0, "About", [](QQmlEngine *engine, QJSEngine *) -> QJSValue {
        return engine->toScriptValue(KAboutData::applicationData());
    });
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "Database", &Database::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "NetworkConnectionManager", &NetworkConnectionManager::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "Fetcher", &Fetcher::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "DataManager", &DataManager::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "SettingsManager", SettingsManager::self());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "DownloadModel", &DownloadModel::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "ErrorLogModel", &ErrorLogModel::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "AudioManager", &AudioManager::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "StorageManager", &StorageManager::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "Sync", &Sync::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "SystrayIcon", &SystrayIcon::instance());

    qmlRegisterUncreatableMetaObject(SyncUtils::staticMetaObject, "org.kde.kasts", 1, 0, "SyncUtils", QStringLiteral("Error: only enums and structs"));

    qRegisterMetaType<Entry *>("const Entry*"); // "hack" to make qml understand Entry*
    qRegisterMetaType<Feed *>("const Feed*"); // "hack" to make qml understand Feed*
    qRegisterMetaType<QVector<SyncUtils::Device>>("QVector<SyncUtils::Device>"); // "hack" to make qml understand QVector of SyncUtils::Device

    // Make sure that settings are saved before the application exits
    QObject::connect(&app, &QCoreApplication::aboutToQuit, SettingsManager::self(), &SettingsManager::save);

    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
