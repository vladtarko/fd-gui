#include <QDebug>
#include <QGuiApplication>
#include <QUrl>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlError>
#include <QtQml>

#include "dmstheme.h"
#include "fdsearch.h"
#include "launcher.h"
#include "previewprovider.h"
#include "resultmodel.h"
#include "settingsstore.h"

int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Material");

    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("fd-gui"));
    QCoreApplication::setApplicationName(QStringLiteral("fd-gui"));

    qRegisterMetaType<ResultItem>("ResultItem");
    qRegisterMetaType<QList<ResultItem>>("QList<ResultItem>");

    ResultModel model;
    SettingsStore settings;
    DmsTheme theme;
    FdSearch search(&model);
    PreviewProvider preview;
    Launcher launcher;

    qmlRegisterSingletonInstance("FdGui", 1, 0, "ResultModel", &model);
    qmlRegisterSingletonInstance("FdGui", 1, 0, "SettingsStore", &settings);
    qmlRegisterSingletonInstance("FdGui", 1, 0, "DmsTheme", &theme);
    qmlRegisterSingletonInstance("FdGui", 1, 0, "FdSearch", &search);
    qmlRegisterSingletonInstance("FdGui", 1, 0, "Preview", &preview);
    qmlRegisterSingletonInstance("FdGui", 1, 0, "Launcher", &launcher);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &errors) {
        for (const QQmlError &error : errors) {
            qWarning().noquote() << error.toString();
        }
    });

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/FdGui/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
