//
// Created by Chow on 2025/7/16.
//
#include <QApplication>
#include "core/IPC.h"

#include <QFile>
#include <QJsonDocument>

#include <iostream>

void MessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    switch (type) {
        case QtInfoMsg:
            std::cout << "Info: " << msg.toStdString() << std::endl;
            break;
        case QtDebugMsg:
            std::cout << "Debug: " << msg.toStdString() << std::endl;
            break;
        case QtWarningMsg:
            std::cout << "Warning: " << msg.toStdString() << std::endl;
            break;
        case QtCriticalMsg:
            std::cout << "Critical: " << msg.toStdString() << std::endl;
            break;
        case QtFatalMsg:
            std::cout << "Fatal: " << msg.toStdString() << std::endl;
            abort();
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    qInstallMessageHandler(MessageHandler);

    IPC ipc;

    return QApplication::exec();
}
