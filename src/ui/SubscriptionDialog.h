#pragma once

#include "domain/Subscription.h"

#include <QDialog>

namespace zarya {

class ZaryaCheckBox;
class ZaryaTextArea;
class ZaryaTextField;

class SubscriptionDialog : public QDialog {
    Q_OBJECT

public:
    static bool editSubscription(QWidget* parent, Subscription& subscription);

private:
    explicit SubscriptionDialog(QWidget* parent, Subscription& subscription);

    Subscription& m_subscription;
    ZaryaTextField* m_nameEdit = nullptr;
    ZaryaTextField* m_urlEdit = nullptr;
    ZaryaCheckBox* m_enabledCheck = nullptr;
    ZaryaTextField* m_userAgentEdit = nullptr;
    ZaryaTextArea* m_remarksEdit = nullptr;
};

} // namespace zarya
