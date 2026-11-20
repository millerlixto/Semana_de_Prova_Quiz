#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "QuizEngine.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QuizEngine quizEngine;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("quizEngine", &quizEngine);

    const QUrl url(QStringLiteral("qrc:/QuizApp/src/frontend/Main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );
    engine.load(url);

    return app.exec();
}