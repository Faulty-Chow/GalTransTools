//
// Created by Chow on 2025/7/16.
//

#include "TransMatcher.h"

#include <execution>
#include <QStandardItemModel>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextDocument>
#include <QLineEdit>
#include <QTextEdit>

class TransMatcherModel : public QStandardItemModel {
    QJsonArray m_origin;
    QMap<QString, QJsonArray> m_trans;

public:
    explicit TransMatcherModel(QObject *parent) : QStandardItemModel(parent) {
        setColumnCount(1);
        setHorizontalHeaderLabels({"Origin"});
    }

    void setOrigin(const QJsonArray &origin) {
        m_origin = origin;
        if (rowCount() < m_origin.size()) {
            setRowCount(m_origin.size());
        }
        emit dataChanged(index(0, 0),
                         index(rowCount() - 1, 0),
                         {Qt::DisplayRole});
    }

    void setTrans(const QString &label, const QJsonArray &trans) {
        if (not m_trans.contains(label)) {
            setColumnCount(columnCount() + 1);
            setHorizontalHeaderItem(columnCount() - 1, new QStandardItem(label));
        }
        if (rowCount() < trans.size()) {
            setRowCount(trans.size());
        }
        m_trans[label] = trans;
        for (int col = 0; col < columnCount(); ++col) {
            if (label == horizontalHeaderItem(col)->text()) {
                emit dataChanged(index(0, col),
                                 index(rowCount() - 1, col),
                                 {Qt::DisplayRole});
            }
        }
    }

    QJsonArray getTrans(const QString &label) const {
        if (not m_trans.contains(label))
            return {};
        return m_trans[label];
    }

    void insertItem(const QModelIndex &idx) {
        int col = idx.column();
        int row = idx.row();
        if (col == 0)
            return;
        QString label = horizontalHeaderItem(col)->text();
        if (not m_trans.contains(label))
            return;
        if (row < m_trans[label].size()) {
            m_trans[label].insert(row, {});
            emit dataChanged(idx, idx.siblingAtRow(rowCount() - 1),
                             {Qt::DisplayRole});
        }
    }

    void removeItem(const QModelIndex &idx) {
        int col = idx.column();
        int row = idx.row();
        if (col == 0)
            return;
        QString label = horizontalHeaderItem(col)->text();
        if (not m_trans.contains(label))
            return;
        if (row < m_trans[label].size()) {
            m_trans[label].removeAt(row);
            emit dataChanged(idx, idx.siblingAtRow(rowCount() - 1),
                             {Qt::DisplayRole});
        }
    }

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override {
        if (!idx.isValid()) return {};
        int col = idx.column();
        int row = idx.row();
        switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                if (col == 0) {
                    if (row < m_origin.size()) {
                        return m_origin[row].toObject();
                    }
                } else {
                    QString label = horizontalHeaderItem(col)->text();
                    if (m_trans.contains(label) and row < m_trans[label].size()) {
                        return m_trans[label][row].toObject();
                    }
                }
                break;
            default:
                return QStandardItemModel::data(idx, role);
        }
        return QStandardItemModel::data(idx, role);
    }

    bool setData(const QModelIndex &idx, const QVariant &value, int role) override {
        if (!idx.isValid()) return false;
        int col = idx.column();
        int row = idx.row();
        switch (role) {
            case Qt::DisplayRole:
                return QStandardItemModel::setData(idx, value, role);
            case Qt::EditRole: {
                if (col == 0)
                    return false;
                QString label = horizontalHeaderItem(col)->text();
                if (not m_trans.contains(label))
                    return false;
                if (row < m_trans[label].size()) {
                    m_trans[label][row] = value.toJsonObject();
                }
                break;
            }
            default:
                return QStandardItemModel::setData(idx, value, role);
        }
        return QStandardItemModel::setData(idx, value, role);
    }

    Qt::ItemFlags flags(const QModelIndex &idx) const override {
        Qt::ItemFlags f = QStandardItemModel::flags(idx);
        int col = idx.column();
        int row = idx.row();
        if (col == 0)
            return f & ~Qt::ItemIsEditable;
        QString label = horizontalHeaderItem(col)->text();
        if (not m_trans.contains(label))
            return Qt::NoItemFlags;
        if (row >= m_trans[label].size())
            return Qt::NoItemFlags;
        return f;
    }
};

class TransMatcherDelegate : public QStyledItemDelegate {
    int m_margin = 5;
    int m_spacing = 4;

public:
    TransMatcherDelegate(QObject *parent)
        : QStyledItemDelegate(parent) {
    }

    QRect nameRect(const QStyleOptionViewItem &option) const {
        QRect rect = option.rect;
        QFont nameFont = option.font;
        nameFont.setBold(true);
        QFontMetrics nameMetrics(nameFont);
        int nameHeight = nameMetrics.height() + 4;
        return QRect{
            rect.left() + m_margin, rect.top() + m_margin,
            rect.width() - 2 * m_margin, nameHeight
        };
    }

    QRect msgRect(const QStyleOptionViewItem &option) const {
        QRect rect = option.rect;
        QRect nameRect = this->nameRect(option);
        return QRect{
            QPoint{
                rect.left() + m_margin,
                nameRect.bottom() + m_spacing
            },
            QSize{
                rect.width() - 2 * m_margin,
                rect.height() - nameRect.height() - m_spacing - 2 * m_margin
            }
        };
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        QJsonObject obj = index.data(Qt::DisplayRole).toJsonObject();
        QString name = obj.value("name").toString();
        QString message = obj.value("message").toString();
        auto nameRect = this->nameRect(option);
        auto msgRect = this->msgRect(option);

        QRect rect = option.rect;
        if (option.state & QStyle::State_Selected)
            painter->fillRect(rect, option.palette.highlight());
        else
            painter->fillRect(rect, option.palette.base());

        // border rect
        painter->save();

        painter->setPen(QPen(Qt::darkGray, 1));
        QRectF borderRect = option.rect.adjusted(1, 1, -1, -1);
        painter->drawRoundedRect(borderRect, 3, 3);

        painter->restore();

        // Separator
        painter->save();

        int lineY = (nameRect.bottom() + msgRect.top()) / 2;
        painter->setPen(QPen(Qt::lightGray, 1));
        painter->drawLine(rect.left() + m_margin, lineY,
                          rect.right() - m_margin, lineY);

        painter->restore();

        // Name
        painter->save();

        QFont nameFont = option.font;
        nameFont.setBold(true);
        painter->setFont(nameFont);
        painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, name);

        painter->restore();

        // Message
        painter->save();

        QFont msgFont = option.font;
        msgFont.setBold(false);
        painter->setFont(msgFont);

        QTextDocument doc;
        doc.setDefaultFont(msgFont);
        doc.setTextWidth(msgRect.width());
        doc.setPlainText(message);
        painter->translate(msgRect.topLeft());
        doc.drawContents(painter);

        painter->restore();
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override {
        QPoint eventPos = parent->mapFromGlobal(QCursor::pos());

        auto nameRect = this->nameRect(option);
        auto msgRect = this->msgRect(option);

        if (nameRect.contains(eventPos)) {
            return new QLineEdit(parent);
        }
        if (msgRect.contains(eventPos)) {
            auto editor = new QTextEdit(parent);
            editor->setWordWrapMode(QTextOption::WordWrap);
            return editor;
        }
        return nullptr;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        QJsonObject obj = index.data(Qt::EditRole).toJsonObject();
        if (auto lineEdit = qobject_cast<QLineEdit *>(editor)) {
            lineEdit->setText(obj["name"].toString());
        } else if (auto textEdit = qobject_cast<QTextEdit *>(editor)) {
            textEdit->setPlainText(obj["message"].toString());
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override {
        QJsonObject obj = index.data(Qt::EditRole).toJsonObject();
        if (auto lineEdit = qobject_cast<QLineEdit *>(editor)) {
            obj["name"] = lineEdit->text();
        } else if (auto textEdit = qobject_cast<QTextEdit *>(editor)) {
            obj["message"] = textEdit->toPlainText();
        }
        model->setData(index, obj, Qt::EditRole);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override {
        if (auto lineEdit = qobject_cast<QLineEdit *>(editor)) {
            lineEdit->setGeometry(nameRect(option));
        } else if (auto textEdit = qobject_cast<QTextEdit *>(editor)) {
            textEdit->setGeometry(msgRect(option));
        }
    }
};

class TransMatcherPrivate {
    TransMatcher *m_p;
    TransMatcherModel *m_model;

public:
    TransMatcherPrivate(TransMatcher *p): m_p(p), m_model(new TransMatcherModel(p)) {
        m_p->setModel(m_model);
        m_p->setItemDelegate(new TransMatcherDelegate(p));

        m_p->setWordWrap(false);
        m_p->verticalHeader()->setDefaultSectionSize(100);
    }

    void setOrigin(const QJsonArray &origin) {
        m_model->setOrigin(origin);
    }

    void setTrans(const QString &label, const QJsonArray &trans) {
        m_model->setTrans(label, trans);
    }

    QJsonArray getTrans(const QString &label) const {
        return m_model->getTrans(label);
    }

    void insertItem(const QModelIndex &idx) {
        m_model->insertItem(idx);
    }

    void removeItem(const QModelIndex &idx) {
        m_model->removeItem(idx);
    }
};

TransMatcher::TransMatcher(QWidget *parent): m_private(new TransMatcherPrivate(this)) {
}

TransMatcher::~TransMatcher() {
    delete m_private;
}

void TransMatcher::setOrigin(const QJsonArray &origin) {
    m_private->setOrigin(origin);
}

void TransMatcher::setTrans(const QString &label, const QJsonArray &trans) {
    m_private->setTrans(label, trans);
}

QJsonArray TransMatcher::getTrans(const QString &label) const {
    return m_private->getTrans(label);
}

void TransMatcher::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);

    QAction *insertAction = menu.addAction("Insert Item");
    QObject::connect(insertAction, &QAction::triggered, [this]() {
        m_private->insertItem(currentIndex());
    });

    QAction *removeAction = menu.addAction("Remove Item");
    QObject::connect(removeAction, &QAction::triggered, [this]() {
        m_private->removeItem(currentIndex());
    });

    menu.exec(event->globalPos());
}
