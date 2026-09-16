#pragma once

#include <QMessageBox>
#include <QWidget>
#include <QString>

namespace Notify {

inline void error(QWidget *parent, const QString &message) {
    QMessageBox::warning(parent, QObject::tr("Error"), message);
}

inline void info(QWidget *parent, const QString &message) {
    QMessageBox::information(parent, QObject::tr("SyncMark"), message);
}

inline bool confirm(QWidget *parent, const QString &title, const QString &message) {
    return QMessageBox::question(parent, title, message,
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) == QMessageBox::Yes;
}

} // namespace Notify
