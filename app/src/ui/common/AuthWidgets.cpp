#include "AuthWidgets.h"
#include "../Theme.h"

#include <QLabel>
#include <QPushButton>
#include <QFont>

QLabel *AuthWidgets::icon(QWidget *parent, int size) {
    auto *label = new QLabel(parent);
    label->setPixmap(Theme::appIcon().pixmap(size, size));
    label->setAlignment(Qt::AlignCenter);
    label->setFixedHeight(size);
    return label;
}

QLabel *AuthWidgets::title(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setObjectName("authTitle");
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    QFont f = label->font();
    f.setPointSize(f.pointSize() + 8);
    f.setBold(true);
    label->setFont(f);
    return label;
}

QLabel *AuthWidgets::subtitle(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setObjectName("authSubtitle");
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setStyleSheet("color: palette(disabled-text);");
    return label;
}

QLabel *AuthWidgets::fieldLabel(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    QFont f = label->font();
    f.setBold(true);
    f.setPointSize(f.pointSize() - 1);
    label->setFont(f);
    label->setStyleSheet("color: palette(disabled-text);");
    return label;
}

QPushButton *AuthWidgets::primaryButton(const QString &text, QWidget *parent) {
    auto *button = new QPushButton(text, parent);
    button->setDefault(true);
    button->setAutoDefault(true);
    button->setMinimumHeight(40);
    QFont f = button->font();
    f.setBold(true);
    button->setFont(f);
    return button;
}

QPushButton *AuthWidgets::linkButton(const QString &text, QWidget *parent) {
    auto *button = new QPushButton(text, parent);
    button->setFlat(true);
    button->setCursor(Qt::PointingHandCursor);
    const QString accent = Theme::accentColor().name();
    button->setStyleSheet(QStringLiteral(
        "QPushButton { border: none; background: transparent; color: %1; padding: 6px; }"
        "QPushButton:hover { text-decoration: underline; }")
        .arg(accent));
    return button;
}

QLabel *AuthWidgets::messageLabel(QWidget *parent) {
    auto *label = new QLabel(parent);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    label->setVisible(false);
    return label;
}

void AuthWidgets::showError(QLabel *label, const QString &text) {
    if (text.isEmpty()) { clearMessage(label); return; }
    label->setText(text);
    label->setStyleSheet(
        "background: rgba(214, 69, 69, 0.12); color: #d64545;"
        "border: 1px solid rgba(214, 69, 69, 0.35); border-radius: 8px; padding: 8px 12px;");
    label->setVisible(true);
}

void AuthWidgets::showInfo(QLabel *label, const QString &text) {
    if (text.isEmpty()) { clearMessage(label); return; }
    label->setText(text);
    label->setStyleSheet(
        "background: rgba(76, 110, 245, 0.10); color: palette(text);"
        "border: 1px solid rgba(76, 110, 245, 0.30); border-radius: 8px; padding: 8px 12px;");
    label->setVisible(true);
}

void AuthWidgets::clearMessage(QLabel *label) {
    label->clear();
    label->setStyleSheet({});
    label->setVisible(false);
}
