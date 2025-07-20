//
// Created by Chow on 2025/7/20.
//

#include "IPC.h"

#include <qjsondocument.h>

#include "widgets/TransMatcher.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QJsonObject>
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>

class IPCPrivate {
    IPC *m_p;
    QTcpServer *m_server;

    struct ClientResource {
        QByteArray buffer;
    };

    QMap<QTcpSocket *, std::shared_ptr<ClientResource> > m_clients;

public:
    IPCPrivate(IPC *p): m_p(p), m_server(new QTcpServer(p)) {
        QObject::connect(m_server, &QTcpServer::newConnection,
                         [this]() { onNewConnection(); });

        if (not m_server->listen(QHostAddress::Any, 12345)) {
            qCritical() << "Failed to start IPC server:" << m_server->errorString();
        }
        qInfo() << "IPC server started on port" << m_server->serverPort();
    }

private:
    void onNewConnection() {
        QTcpSocket *conn = m_server->nextPendingConnection();
        if (nullptr == conn) {
            qWarning() << "Failed to get pending connection";
            return;
        }
        m_clients[conn] = std::make_shared<ClientResource>();
        qInfo() << "New connection from" << conn->peerAddress().toString()
                << ":" << conn->peerPort();
        QObject::connect(conn, &QTcpSocket::disconnected,
                         [this,conn]() {
                             // qInfo() << "Connection disconnected from"
                             //         << conn->peerAddress().toString() << ":"
                             //         << conn->peerPort();
                             // m_clients.remove(conn);
                             conn->deleteLater();
                         });
        QObject::connect(conn, &QTcpSocket::readyRead,
                         [this,conn]() {
                             onReadyRead(conn);
                         });
    }

    void onReadyRead(QTcpSocket *conn) {
        if (not m_clients.contains(conn)) {
            qWarning() << "Connection not found in clients map";
            return;
        }
        auto resource = m_clients[conn];

        resource->buffer.append(conn->readAll());
        QJsonParseError parseError;
        auto doc = QJsonDocument::fromJson(resource->buffer, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            return;
        }
        qInfo() << "Received from"
                << conn->peerAddress().toString() << ":"
                << conn->peerPort();

        const auto writeResponse =
                [conn](QJsonDocument json) {
            if (conn->write(json.toJson()) == -1) {
                qWarning() << "Failed to write response to client";
                // TODO
            }
        };

        QJsonObject received;
        received["status"] = "received";
        writeResponse(QJsonDocument(received));

        QJsonObject obj = doc.object();
        QJsonArray origin = obj["origin"].toArray();
        QJsonObject trans = obj["trans"].toObject();
        QString trans_label = trans["label"].toString();
        QJsonArray trans_data = trans["data"].toArray();

        int ret = [&]() {
            QDialog dialog;
            auto vLayout = new QVBoxLayout(&dialog);
            dialog.setLayout(vLayout);

            auto matcher = new TransMatcher(&dialog);
            matcher->setOrigin(origin);
            matcher->setTrans(trans_label, trans_data);
            vLayout->addWidget(matcher);

            auto btnLayout = new QHBoxLayout;
            auto acceptBtn = new QPushButton("Accept", &dialog);
            QObject::connect(acceptBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
            btnLayout->addWidget(acceptBtn);
            auto rejectBtn = new QPushButton("Reject", &dialog);
            QObject::connect(rejectBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
            btnLayout->addWidget(rejectBtn);
            vLayout->addLayout(btnLayout);

            if (QDialog::Accepted == dialog.exec()) {
                trans_data = matcher->getTrans(trans_label);
                return 0;
            }
            return -1;
        }();
        if (0 == ret) {
            QJsonObject accepted;
            accepted["status"] = "accepted";
            accepted["trans"] = trans_data;
            writeResponse(QJsonDocument(accepted));
        } else {
            QJsonObject rejected;
            rejected["status"] = "rejected";
            writeResponse(QJsonDocument(rejected));
        }

        qInfo() << "Processed request from"
                << conn->peerAddress().toString() << ":"
                << conn->peerPort();
    }
};

IPC::IPC(): m_private(new IPCPrivate(this)) {
}

IPC::~IPC() {
    delete m_private;
}
