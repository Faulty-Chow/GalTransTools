//
// Created by Chow on 2025/7/16.
//

#ifndef TRANSMATCHER_H
#define TRANSMATCHER_H

#include <QTableView>
#include <QJsonArray>

class QContextMenuEvent;

class TransMatcher : public QTableView {
    Q_OBJECT

public:
    TransMatcher(QWidget *parent = nullptr);

    ~TransMatcher();

    void setOrigin(const QJsonArray &origin);

    void setTrans(const QString &label, const QJsonArray &trans);

    QJsonArray getTrans(const QString &label) const;

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    friend class TransMatcherPrivate;
    TransMatcherPrivate *m_private;
};


#endif //TRANSMATCHER_H
