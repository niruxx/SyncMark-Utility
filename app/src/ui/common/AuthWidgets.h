#pragma once

#include <QString>

class QWidget;
class QLabel;
class QPushButton;
class QLineEdit;

// Shared building blocks for the pre-login "card" screens (ConnectDialog,
// LoginDialog, SetupDialog) so all three look like one consistent flow
// instead of three independently-styled QFormLayout dialogs.
namespace AuthWidgets {

// The app mark, centered, used as a small logo at the top of each card.
QLabel *icon(QWidget *parent, int size = 64);

// Large centered heading, e.g. "Welcome to SyncMark".
QLabel *title(const QString &text, QWidget *parent);

// Muted centered line under the heading.
QLabel *subtitle(const QString &text, QWidget *parent);

// Small bold label placed above a field (top-aligned form style).
QLabel *fieldLabel(const QString &text, QWidget *parent);

// Full-width accent-colored call-to-action button (set as the dialog's
// default/autoDefault button so Enter submits and the theme's `:default`
// QSS rule colors it).
QPushButton *primaryButton(const QString &text, QWidget *parent);

// Borderless, underlined, accent-colored "text link" button for secondary
// actions (Cancel/Back) so they don't visually compete with the primary CTA.
QPushButton *linkButton(const QString &text, QWidget *parent);

// A status/error line rendered as a soft rounded "chip" instead of bare
// colored text. Starts empty/invisible-looking; use showError/showInfo/clear.
QLabel *messageLabel(QWidget *parent);
void showError(QLabel *label, const QString &text);
void showInfo(QLabel *label, const QString &text);
void clearMessage(QLabel *label);

} // namespace AuthWidgets
