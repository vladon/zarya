#pragma once

#include "domain/Subscription.h"

#include <QDialog>

namespace Ui {
class Checkbox;
class InputField;
} // namespace Ui

namespace zarya {

class SubscriptionDialog : public QDialog {
    Q_OBJECT

public:
    static bool editSubscription(QWidget* parent, Subscription& subscription);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    explicit SubscriptionDialog(QWidget* parent, Subscription& subscription);
    void acceptChanges();

    Subscription& m_subscription;
    Ui::InputField* m_nameEdit = nullptr;
    Ui::InputField* m_urlEdit = nullptr;
    Ui::Checkbox* m_enabledCheck = nullptr;
    Ui::InputField* m_userAgentEdit = nullptr;
    Ui::InputField* m_remarksEdit = nullptr;
};

} // namespace zarya
