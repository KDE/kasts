/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */


#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickStyle>

#ifdef Q_OS_ANDROID
#include <QGuiApplication>
#else
#include <QApplication>
#endif

#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>

#include "alligatorsettings.h"
#include "database.h"
#include "entryListModel.h"
#include "feedListModel.h"
#include "fetcher.h"

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
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));

    qmlRegisterType<FeedListModel>("org.kde.alligator", 1, 0, "FeedListModel");
    qmlRegisterType<EntryListModel>("org.kde.alligator", 1, 0, "EntryListModel");
    qmlRegisterSingletonType<Fetcher>("org.kde.alligator", 1, 0, "Fetcher", [](QQmlEngine *engine, QJSEngine *) -> QObject * {
        engine->setObjectOwnership(&Fetcher::instance(), QQmlEngine::CppOwnership);
        return &Fetcher::instance();
    });
    qmlRegisterSingletonType<Database>("org.kde.alligator", 1, 0, "Database", [](QQmlEngine *engine, QJSEngine *) -> QObject * {
        engine->setObjectOwnership(&Database::instance(), QQmlEngine::CppOwnership);
        return &Database::instance();
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("RSS/Atom Feed Reader"));

    KAboutData about(QStringLiteral("alligator"), i18n("Alligator"), QStringLiteral("0.1"), i18n("Feed Reader"), KAboutLicense::GPL, i18n("© 2020 KDE Community"));
    about.addAuthor(i18n("Tobias Fella"), QString(), QStringLiteral("fella@posteo.de"));
    KAboutData::setApplicationData(about);

    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    engine.rootContext()->setContextProperty(QStringLiteral("_aboutData"), QVariant::fromValue(about));

    AlligatorSettings settings;

    engine.rootContext()->setContextProperty(QStringLiteral("_settings"), &settings);

    Database::instance();

    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
