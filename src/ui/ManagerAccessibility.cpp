#include "ui/ManagerAccessibility.h"

#include <QEvent>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QWidget>

namespace zarya {
namespace {

void focusWhenVisible(const QPointer<QWidget>& target)
{
    if (target && target->isVisible()) {
        target->setFocus(Qt::OtherFocusReason);
    }
}

class InitialFocusAfterShow final : public QObject {
public:
    InitialFocusAfterShow(QWidget* window, QWidget* target)
        : QObject(window)
        , m_window(window)
        , m_target(target)
    {
        m_window->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_window && event->type() == QEvent::Show) {
            m_window->removeEventFilter(this);
            QTimer::singleShot(0, m_target, [target = m_target] {
                focusWhenVisible(target);
            });
            deleteLater();
        }
        return false;
    }

private:
    QWidget* m_window = nullptr;
    QPointer<QWidget> m_target;
};

} // namespace

void scheduleManagerInitialFocus(QWidget* focusTarget)
{
    if (!focusTarget) {
        return;
    }

    QWidget* window = focusTarget->window();
    if (window->isVisible()) {
        QTimer::singleShot(0, focusTarget, [target = QPointer<QWidget>(focusTarget)] {
            focusWhenVisible(target);
        });
        return;
    }
    new InitialFocusAfterShow(window, focusTarget);
}

} // namespace zarya
