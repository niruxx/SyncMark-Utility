#include "Animations.h"

#include <QWidget>
#include <QDialog>
#include <QCoreApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QEvent>

void Animations::fadeIn(QWidget *widget, int durationMs) {
    if (!widget) return;

    auto *effect = new QGraphicsOpacityEffect(widget);
    effect->setOpacity(0.0);
    widget->setGraphicsEffect(effect);

    auto *animation = new QPropertyAnimation(effect, "opacity", widget);
    animation->setDuration(durationMs);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    // Drop the effect once the fade completes: leaving a QGraphicsOpacityEffect
    // installed permanently can clip or mis-position child popups (combo box
    // drop-downs, tooltips), so it should only exist for the duration of the fade.
    QObject::connect(animation, &QPropertyAnimation::finished, widget, [widget]() {
        widget->setGraphicsEffect(nullptr);
    });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

namespace {

class DialogFadeFilter : public QObject {
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Show) {
            if (auto *dialog = qobject_cast<QDialog *>(watched))
                Animations::fadeIn(dialog, 160);
        }
        return QObject::eventFilter(watched, event);
    }
};

} // namespace

void Animations::installGlobalDialogFade(QCoreApplication *app) {
    if (!app) return;
    app->installEventFilter(new DialogFadeFilter(app));
}
