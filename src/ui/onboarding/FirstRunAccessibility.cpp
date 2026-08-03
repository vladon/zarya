#include "ui/onboarding/FirstRunAccessibility.h"

#include <QAccessible>
#include <QAccessibleEvent>
#include <QWidget>

namespace zarya {

void activateFirstRunPageAccessibility(QWidget* page, QWidget* focusTarget)
{
    if (!page || !page->isVisible()) {
        return;
    }
    if (focusTarget) {
        QWidget* resolvedTarget = focusTarget;
        while (resolvedTarget->focusPolicy() == Qt::NoFocus
               && resolvedTarget->focusProxy()
               && resolvedTarget->focusProxy() != resolvedTarget) {
            resolvedTarget = resolvedTarget->focusProxy();
        }
        resolvedTarget->setFocus(Qt::OtherFocusReason);
    }
    const QString title = page->accessibleName();
    if (!title.isEmpty()) {
        QAccessibleAnnouncementEvent event(page, title);
        event.setPoliteness(QAccessible::AnnouncementPoliteness::Polite);
        QAccessible::updateAccessibility(&event);
    }
}

} // namespace zarya
