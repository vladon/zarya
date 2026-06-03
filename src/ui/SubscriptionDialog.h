#pragma once

#include "domain/Subscription.h"

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QPlainTextEdit;

namespace zarya {

class SubscriptionDialog : public QDialog {
    Q_OBJECT

public:
    static bool editSubscription(QWidget* parent, Subscription& subscription);

private:
    explicit SubscriptionDialog(QWidget* parent, Subscription& subscription);

    Subscription& m_subscription;
    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_urlEdit = nullptr;
    QCheckBox* m_enabledCheck = nullptr;
    QLineEdit* m_userAgentEdit = nullptr;
    QPlainTextEdit* m_remarksEdit = nullptr;
};

} // namespace zarya
