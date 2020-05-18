/**
 * Copyright 2020 Tobias Fella <fella@posteo.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef Q_OS_ANDROID
#include <QGuiApplication>
#else
#include <QApplication>
#endif

#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickView>

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
#else
    QApplication app(argc, argv);
#endif

    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationName(QStringLiteral("Alligator"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));

    qmlRegisterType<FeedListModel>("org.kde.alligator", 1, 0, "FeedListModel");
    qmlRegisterType<EntryListModel>("org.kde.alligator", 1, 0, "EntryListModel");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("RSS Feed Reader"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QLatin1String("url"), i18n("Url to add to the subscriptions"));
    parser.process(app);

    if (parser.positionalArguments().size() == 1) {
        QString url = parser.positionalArguments().at(0);
        qDebug() << url;
        // TODO
    }

    KAboutData about(QStringLiteral("alligator"), i18n("Alligator"), QStringLiteral("0.1"), i18n("Feed Reader"), KAboutLicense::GPL, i18n("© 2020 KDE Community"));
    about.addAuthor(i18n("Tobias Fella"), QString(), QStringLiteral("fella@posteo.de"));
    KAboutData::setApplicationData(about);

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
