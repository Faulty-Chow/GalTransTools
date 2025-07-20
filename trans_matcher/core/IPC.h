//
// Created by Chow on 2025/7/20.
//

#ifndef IPC_H
#define IPC_H

#include <QObject>

class IPC : public QObject {
public:
    enum Status {
        Accepted,
        Rejected,
        Finished,
    };

    IPC();

    ~IPC();

private:
    friend class IPCPrivate;
    IPCPrivate *m_private;
};


#endif //IPC_H
