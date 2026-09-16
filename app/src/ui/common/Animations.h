#pragma once

class QWidget;
class QCoreApplication;

// Small, centralized helpers so motion in the app is consistent instead of
// each dialog/page inventing its own timing/easing. Kept deliberately
// simple (opacity fades only) so it stays stable across every widget type
// in the app rather than fighting Qt's layout/paint system.
namespace Animations {

// Fades `widget` in from transparent to opaque. The graphics effect used to
// do this is removed again once the animation finishes, so it never lingers
// and interferes with child popups (e.g. QComboBox) that are sensitive to
// an ancestor having a QGraphicsEffect installed.
void fadeIn(QWidget *widget, int durationMs = 180);

// Installs an application-wide event filter that fades in every QDialog the
// moment it's first shown (covers every dialog in the app with no per-file
// changes needed). Call once, near startup.
void installGlobalDialogFade(QCoreApplication *app);

} // namespace Animations
