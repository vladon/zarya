#include "platform/macos/MacMessageBoxEscape.h"

#include <QAbstractButton>
#include <QApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QWidget>

#import <AppKit/AppKit.h>

namespace zarya {
namespace {

QAbstractButton* dismissButtonForMessageBox(QMessageBox* box)
{
    if (!box) {
        return nullptr;
    }
    if (QAbstractButton* button = box->escapeButton()) {
        return button;
    }
    if (QAbstractButton* button = box->button(QMessageBox::Cancel)) {
        return button;
    }
    if (QAbstractButton* button = box->button(QMessageBox::Close)) {
        return button;
    }
    if (QAbstractButton* button = box->button(QMessageBox::No)) {
        return button;
    }
    if (QAbstractButton* button = box->button(QMessageBox::Abort)) {
        return button;
    }

    const QList<QAbstractButton*> buttons = box->buttons();
    if (buttons.size() == 1) {
        return buttons.first();
    }

    QAbstractButton* rejectOrNo = nullptr;
    for (QAbstractButton* button : buttons) {
        const QMessageBox::ButtonRole role = box->buttonRole(button);
        if (role == QMessageBox::RejectRole || role == QMessageBox::NoRole) {
            if (rejectOrNo) {
                return nullptr;
            }
            rejectOrNo = button;
        }
    }
    return rejectOrNo;
}

} // namespace

void installMacMessageBoxEscapeMonitor()
{
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;

    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                          handler:^NSEvent*(NSEvent* event) {
        // kVK_Escape = 53
        if (event.keyCode != 53) {
            return event;
        }

        QWidget* modal = QApplication::activeModalWidget();
        auto* box = qobject_cast<QMessageBox*>(modal);
        if (!box) {
            return event;
        }

        QAbstractButton* button = dismissButtonForMessageBox(box);
        if (!button) {
            return event;
        }

        // Queued so we leave the AppKit event callback before closing the dialog.
        QMetaObject::invokeMethod(button, "click", Qt::QueuedConnection);
        return nil;
    }];
}

} // namespace zarya
