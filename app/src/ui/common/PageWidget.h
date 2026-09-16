#pragma once

#include <QWidget>

// Common base for main-window content pages. reload() is called every time
// the page becomes visible so it can (re)fetch data from the server.
class PageWidget : public QWidget {
    Q_OBJECT
public:
    explicit PageWidget(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual void reload() {}
};
