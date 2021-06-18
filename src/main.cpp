/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
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
#include "downloadprogressmodel.h"
#include "entriesmodel.h"
#include "episodemodel.h"
#include "errorlogmodel.h"
#include "feedsmodel.h"
#include "fetcher.h"
#include "kasts-version.h"
#include "mpris2/mpris2.h"
#include "queuemodel.h"
#include "settingsmanager.h"

#ifdef Q_OS_ANDROID
Q_DECL_EXPORT
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    QGuiApplication app(argc, argv);
    qInstallMessageHandler(myMessageHandler);
    QLoggingCategory::setFilterRules(QStringLiteral("org.kde.*=true"));
    QQuickStyle::setStyle(QStringLiteral("Material"));
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
                     i18n("© 2020-2021 KDE Community"));
    about.addAuthor(i18n("Tobias Fella"), QString(), QStringLiteral("fella@posteo.de"));
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

    engine.rootContext()->setContextProperty(QStringLiteral("_aboutData"), QVariant::fromValue(about));

    qmlRegisterType<FeedsModel>("org.kde.kasts", 1, 0, "FeedsModel");
    qmlRegisterType<QueueModel>("org.kde.kasts", 1, 0, "QueueModel");
    qmlRegisterType<EpisodeModel>("org.kde.kasts", 1, 0, "EpisodeModel");
    qmlRegisterType<Mpris2>("org.kde.kasts", 1, 0, "Mpris2");

    qmlRegisterUncreatableType<EntriesModel>("org.kde.kasts", 1, 0, "EntriesModel", QStringLiteral("Get from Feed"));
    qmlRegisterUncreatableType<Enclosure>("org.kde.kasts", 1, 0, "Enclosure", QStringLiteral("Only for enums"));

    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "Fetcher", &Fetcher::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "Database", &Database::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "DataManager", &DataManager::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "SettingsManager", SettingsManager::self());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "DownloadProgressModel", &DownloadProgressModel::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "ErrorLogModel", &ErrorLogModel::instance());
    qmlRegisterSingletonInstance("org.kde.kasts", 1, 0, "AudioManager", &AudioManager::instance());

    qRegisterMetaType<Entry *>("const Entry*"); // "hack" to make qml understand Entry*

    // Make sure that settings are saved before the application exits
    QObject::connect(&app, &QCoreApplication::aboutToQuit, SettingsManager::self(), &SettingsManager::save);

    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
