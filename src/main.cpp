/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickView>

#ifdef Q_OS_ANDROID
#include <QGuiApplication>
#else
#include <QApplication>
#endif

#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>

#include "alligator-version.h"
#include "audiomanager.h"
#include "database.h"
#include "datamanager.h"
#include "downloadprogressmodel.h"
#include "entriesmodel.h"
#include "episodemodel.h"
#include "errorlogmodel.h"
#include "feedsmodel.h"
#include "fetcher.h"
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
    QQuickStyle::setStyle(QStringLiteral("Material"));
#else
    QApplication app(argc, argv);
#endif

    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationName(QStringLiteral("Alligator"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    KLocalizedString::setApplicationDomain("alligator");

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("RSS/Atom Feed Reader"));

    KAboutData about(QStringLiteral("alligator"),
                     i18n("Alligator"),
                     QStringLiteral(ALLIGATOR_VERSION_STRING),
                     i18n("Feed Reader"),
                     KAboutLicense::GPL,
                     i18n("© 2020-2021 KDE Community"));
    about.addAuthor(i18n("Tobias Fella"), QString(), QStringLiteral("fella@posteo.de"));
    about.addAuthor(i18n("Bart De Vries"), QString(), QStringLiteral("bart@mogwai.be"));
    KAboutData::setApplicationData(about);

    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    engine.rootContext()->setContextProperty(QStringLiteral("_aboutData"), QVariant::fromValue(about));

    qmlRegisterType<FeedsModel>("org.kde.alligator", 1, 0, "FeedsModel");
    qmlRegisterType<QueueModel>("org.kde.alligator", 1, 0, "QueueModel");
    qmlRegisterType<EpisodeModel>("org.kde.alligator", 1, 0, "EpisodeModel");
    qmlRegisterType<AudioManager>("org.kde.alligator", 1, 0, "AudioManager");
    qmlRegisterType<Mpris2>("org.kde.alligator", 1, 0, "Mpris2");

    qmlRegisterUncreatableType<EntriesModel>("org.kde.alligator", 1, 0, "EntriesModel", QStringLiteral("Get from Feed"));
    qmlRegisterUncreatableType<Enclosure>("org.kde.alligator", 1, 0, "Enclosure", QStringLiteral("Only for enums"));

    qmlRegisterSingletonInstance("org.kde.alligator", 1, 0, "Fetcher", &Fetcher::instance());
    qmlRegisterSingletonInstance("org.kde.alligator", 1, 0, "Database", &Database::instance());
    qmlRegisterSingletonInstance("org.kde.alligator", 1, 0, "DataManager", &DataManager::instance());
    qmlRegisterSingletonInstance("org.kde.alligator", 1, 0, "SettingsManager", SettingsManager::self());
    qmlRegisterSingletonInstance("org.kde.alligator", 1, 0, "DownloadProgressModel", &DownloadProgressModel::instance());
    qmlRegisterSingletonInstance("org.kde.alligator", 1, 0, "ErrorLogModel", &ErrorLogModel::instance());

    qRegisterMetaType<Entry *>("const Entry*"); // "hack" to make qml understand Entry*

    // Make sure that settings are saved before the application exits
    QObject::connect(&app, &QCoreApplication::aboutToQuit, SettingsManager::self(), &SettingsManager::save);

    Database::instance();
    DataManager::instance();
    DownloadProgressModel::instance();
    ErrorLogModel::instance();

    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
