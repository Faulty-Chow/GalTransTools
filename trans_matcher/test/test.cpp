//
// Created by 87431 on 2025/7/21.
//
#include <QApplication>
#include "widgets/TransMatcher.h"

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
    qInstallMessageHandler(MessageHandler);

    TransMatcher matcher;

    QFile origFile("D:\\Source\\GalTransTools\\trans_api\\data\\orig\\00000001.csv.json");
    if (origFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(origFile.readAll());
        matcher.setOrigin(doc.array());
        origFile.close();
    } else{
        qFatal("Failed to open origin file: %s", qPrintable(origFile.errorString()));
    }

    QFile transFile("D:\\Source\\GalTransTools\\trans_api\\data\\trans\\00000001.csv.json");
    if(transFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(transFile.readAll());
        matcher.setTrans("00000001.csv", doc.array());
        transFile.close();
    } else {
        qFatal("Failed to open translation file: %s", qPrintable(transFile.errorString()));
    }

    matcher.show();

    return QApplication::exec();
}