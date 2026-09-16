#include "Theme.h"

#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QPixmap>
#include <QPainter>
#include <QLinearGradient>
#include <QFont>

namespace {
constexpr const char *kAccentHex = "#4C6EF5";
constexpr const char *kAccentDarkHex = "#7C93FA"; // slightly lighter, for dark backgrounds
}

QColor Theme::accentColor() {
    return QColor(kAccentHex);
}

void Theme::apply(QApplication &app) {
    // Detect the OS preference before we install our own palette.
    const bool dark = app.palette().color(QPalette::Window).lightness() < 128;

    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette pal;
    const QColor accent(dark ? kAccentDarkHex : kAccentHex);

    if (dark) {
        const QColor window(37, 39, 46);
        const QColor base(30, 32, 38);
        const QColor alt(44, 46, 54);
        const QColor text(226, 228, 233);
        const QColor disabledText(120, 122, 128);
        pal.setColor(QPalette::Window, window);
        pal.setColor(QPalette::WindowText, text);
        pal.setColor(QPalette::Base, base);
        pal.setColor(QPalette::AlternateBase, alt);
        pal.setColor(QPalette::ToolTipBase, alt);
        pal.setColor(QPalette::ToolTipText, text);
        pal.setColor(QPalette::Text, text);
        pal.setColor(QPalette::Button, window);
        pal.setColor(QPalette::ButtonText, text);
        pal.setColor(QPalette::BrightText, Qt::red);
        pal.setColor(QPalette::Link, accent);
        pal.setColor(QPalette::Highlight, accent);
        pal.setColor(QPalette::HighlightedText, Qt::white);
        pal.setColor(QPalette::PlaceholderText, disabledText);
        pal.setColor(QPalette::Disabled, QPalette::Text, disabledText);
        pal.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    } else {
        const QColor window(246, 247, 250);
        const QColor base(255, 255, 255);
        const QColor alt(240, 242, 247);
        const QColor text(31, 33, 39);
        const QColor disabledText(150, 152, 158);
        pal.setColor(QPalette::Window, window);
        pal.setColor(QPalette::WindowText, text);
        pal.setColor(QPalette::Base, base);
        pal.setColor(QPalette::AlternateBase, alt);
        pal.setColor(QPalette::ToolTipBase, QColor(60, 62, 68));
        pal.setColor(QPalette::ToolTipText, Qt::white);
        pal.setColor(QPalette::Text, text);
        pal.setColor(QPalette::Button, window);
        pal.setColor(QPalette::ButtonText, text);
        pal.setColor(QPalette::BrightText, Qt::red);
        pal.setColor(QPalette::Link, accent);
        pal.setColor(QPalette::Highlight, accent);
        pal.setColor(QPalette::HighlightedText, Qt::white);
        pal.setColor(QPalette::PlaceholderText, disabledText);
        pal.setColor(QPalette::Disabled, QPalette::Text, disabledText);
        pal.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    }
    app.setPalette(pal);

    QFont font = app.font();
#ifdef Q_OS_WIN
    font.setPointSize(font.pointSize() + 1);
#endif
    app.setFont(font);

    const QString accentHex = accent.name();
    const QString borderHex = dark ? "#3d4049" : "#dde1e8";
    const QString subtleBgHex = dark ? "#2c2e36" : "#f0f2f7";
    const QString hoverBgHex = dark ? "#33353e" : "#eaecf2";

    const QString qss = QStringLiteral(R"(
        QWidget { outline: 0; }

        QToolTip {
            border: 1px solid %1;
            padding: 4px 8px;
            border-radius: 6px;
        }

        QPushButton, QToolButton {
            background: %3;
            border: 1px solid %2;
            border-radius: 7px;
            padding: 6px 14px;
            min-height: 18px;
        }
        QPushButton:hover, QToolButton:hover {
            background: %4;
            border-color: %1;
        }
        QPushButton:pressed, QToolButton:pressed {
            background: %1;
            color: white;
        }
        QPushButton:disabled, QToolButton:disabled {
            color: palette(disabled-text);
        }
        QPushButton:default {
            background: %1;
            color: white;
            border: 1px solid %1;
            font-weight: 600;
        }
        QPushButton:default:hover {
            background: %1;
        }
        QToolButton::menu-indicator { subcontrol-position: right center; }

        QLineEdit, QPlainTextEdit, QTextEdit, QComboBox, QSpinBox, QDateEdit, QDateTimeEdit {
            border: 1px solid %2;
            border-radius: 6px;
            padding: 4px 8px;
            background: palette(base);
            selection-background-color: %1;
        }
        QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus,
        QSpinBox:focus, QDateEdit:focus, QDateTimeEdit:focus {
            border: 1px solid %1;
        }
        QComboBox::drop-down { border: none; width: 20px; }

        QGroupBox {
            border: 1px solid %2;
            border-radius: 8px;
            margin-top: 14px;
            padding-top: 10px;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
            color: %1;
        }

        QTableWidget, QTreeWidget, QListWidget {
            border: 1px solid %2;
            border-radius: 8px;
            gridline-color: %2;
            selection-background-color: %1;
            selection-color: white;
            alternate-background-color: %3;
        }
        QHeaderView::section {
            background: %3;
            border: none;
            border-bottom: 1px solid %2;
            padding: 6px 8px;
            font-weight: 600;
        }
        QTableWidget::item, QTreeWidget::item, QListWidget::item {
            padding: 4px;
            border: none;
        }
        QTreeWidget::item, QListWidget::item { padding: 6px; }
        QTableWidget::item:selected, QTreeWidget::item:selected, QListWidget::item:selected {
            background: %1;
            color: white;
            border-radius: 4px;
        }

        QTabWidget::pane {
            border: 1px solid %2;
            border-radius: 8px;
            top: -1px;
        }
        QTabBar::tab {
            background: transparent;
            padding: 7px 14px;
            margin-right: 2px;
            border-top-left-radius: 7px;
            border-top-right-radius: 7px;
        }
        QTabBar::tab:selected {
            background: %1;
            color: white;
            font-weight: 600;
        }
        QTabBar::tab:!selected:hover { background: %4; }

        QMenu {
            border: 1px solid %2;
            border-radius: 8px;
            padding: 4px;
            background: palette(base);
        }
        QMenu::item {
            padding: 6px 20px;
            border-radius: 5px;
        }
        QMenu::item:selected { background: %1; color: white; }
        QMenu::separator { height: 1px; background: %2; margin: 4px 6px; }

        QSplitter::handle { background: %2; }
        QSplitter::handle:horizontal { width: 2px; }
        QSplitter::handle:vertical { height: 2px; }

        QWidget#topBar {
            background: palette(base);
            border-bottom: 1px solid %2;
        }
        QLabel#topBarSubtle { color: palette(disabled-text); }
        QToolButton#accountButton {
            background: transparent;
            border: 1px solid %2;
            border-radius: 7px;
            padding: 6px 12px;
        }
        QToolButton#accountButton:hover { background: %4; border-color: %1; }
        QToolButton#accountButton::menu-indicator { image: none; width: 0; }

        QScrollBar:vertical {
            background: transparent;
            width: 11px;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: %2;
            border-radius: 5px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover { background: %1; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal {
            background: transparent;
            height: 11px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal {
            background: %2;
            border-radius: 5px;
            min-width: 24px;
        }
        QScrollBar::handle:horizontal:hover { background: %1; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

        QListWidget#navList {
            border: none;
            background: transparent;
            font-size: 13pt;
            padding: 4px;
        }
        QListWidget#navList::item {
            padding: 9px 12px;
            border-radius: 8px;
            margin: 2px 4px;
        }
        QListWidget#navList::item:hover:!selected { background: %4; }
        QListWidget#navList::item:selected {
            background: %1;
            color: white;
            font-weight: 600;
        }

        QWidget#sidebarPanel { background: %3; }
        QLabel#brandTitle { font-size: 15pt; font-weight: 700; }
        QLabel#errorLabel { color: #d64545; }
        QLabel#statTileValue { font-size: 22pt; font-weight: 700; color: %1; }
    )")
        .arg(accentHex, borderHex, subtleBgHex, hoverBgHex);

    app.setStyleSheet(qss);
}

QIcon Theme::appIcon() {
    const int size = 256;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(0, 0, size, size);
    gradient.setColorAt(0, QColor("#6C8CF5"));
    gradient.setColorAt(1, QColor("#3F5FE0"));
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, size, size, size * 0.22, size * 0.22);

    painter.setPen(Qt::white);
    QFont font("Segoe UI", size * 0.5, QFont::Bold);
    if (!QFontInfo(font).exactMatch())
        font = QFont(QApplication::font().family(), size * 0.5, QFont::Bold);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "S");
    painter.end();

    return QIcon(pixmap);
}

QIcon Theme::dotIcon(const QColor &color, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, size, size);
    painter.end();
    return QIcon(pixmap);
}
