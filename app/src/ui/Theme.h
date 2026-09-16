#pragma once

#include <QString>
#include <QIcon>

class QApplication;

// Application-wide look and feel: a Fusion-based palette (light or dark,
// following the OS setting) plus a QSS stylesheet layered on top for
// rounded controls, accent-colored primary actions, and cleaner tables.
// There are no external image assets - the app icon and small glyph icons
// used around the UI are painted at runtime.
namespace Theme {

// Applies the palette + stylesheet + default font to the whole application.
void apply(QApplication &app);

// A simple painted "S" mark in the accent color, used as the window/taskbar
// icon and in the sidebar header.
QIcon appIcon();

// A colored dot/badge icon, used for small inline status markers.
QIcon dotIcon(const QColor &color, int size = 10);

// The accent color currently in use (for code that needs to match it, e.g.
// custom-painted highlights).
QColor accentColor();

} // namespace Theme
