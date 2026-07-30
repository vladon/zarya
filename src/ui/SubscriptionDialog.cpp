#include "ui/SubscriptionDialog.h"

#include "ui/desktopapp/ZaryaControls.h"

#include "styles/style_layers.h"
#include "ui/qt_object_factory.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMetaObject>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <rpl/rpl.h>

namespace zarya {
namespace {

constexpr int kDialogWidth = 560;
constexpr int kContentMargin = 24;
constexpr int kContentSpacing = 12;
constexpr int kRemarksHeight = 96;

} // namespace

SubscriptionDialog::SubscriptionDialog(QWidget* parent, Subscription& subscription)
    : QDialog(parent)
    , m_subscription(subscription)
{
    setWindowTitle(subscription.name.isEmpty() ? tr("Subscription")
                                               : subscription.name);
    setAccessibleName(windowTitle());
    setModal(true);

    auto name = makeZaryaInputField(this, tr("Name"), subscription.name);
    m_nameEdit = name.data();
    m_nameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto url = makeZaryaInputField(this, QStringLiteral("URL"), subscription.url);
    m_urlEdit = url.data();
    m_urlEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto enabled = makeZaryaCheckbox(this, tr("Enabled"), subscription.enabled);
    m_enabledCheck = enabled.data();
    m_enabledCheck->installEventFilter(this);

    auto userAgent =
        makeZaryaInputField(this, tr("User-Agent"), subscription.userAgent);
    m_userAgentEdit = userAgent.data();
    m_userAgentEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* userAgentHint = Ui::CreateChild<Ui::FlatLabel>(
        this,
        tr("Optional; default Zarya User-Agent"),
        st::boxLabel);
    userAgentHint->setAccessibleName(tr("Optional; default Zarya User-Agent"));

    auto remarks = makeZaryaInputField(
        this,
        tr("Remarks"),
        subscription.remarks,
        ZaryaInputMode::MultiLine);
    m_remarksEdit = remarks.data();
    m_remarksEdit->setMinHeight(kRemarksHeight);
    m_remarksEdit->setMaxHeight(kRemarksHeight);
    m_remarksEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* actionRow = new QWidget(this);
    auto* actionLayout = new QHBoxLayout(actionRow);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    actionLayout->addStretch();

    auto cancel = makeZaryaButton(
        actionRow,
        tr("Cancel"),
        ZaryaButtonRole::Secondary);
    auto* cancelButton = cancel.data();
    actionLayout->addWidget(cancel.release());
    cancelButton->setClickedCallback([this] {
        QMetaObject::invokeMethod(this, [this] { reject(); }, Qt::QueuedConnection);
    });

    auto save = makeZaryaButton(
        actionRow,
        tr("Save"),
        ZaryaButtonRole::Primary);
    auto* saveButton = save.data();
    actionLayout->addWidget(save.release());
    saveButton->setClickedCallback([this] {
        QMetaObject::invokeMethod(
            this,
            [this] { acceptChanges(); },
            Qt::QueuedConnection);
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(
        kContentMargin,
        kContentMargin,
        kContentMargin,
        kContentMargin);
    layout->setSpacing(kContentSpacing);
    layout->addWidget(name.release());
    layout->addWidget(url.release());
    layout->addWidget(enabled.release());
    layout->addWidget(userAgent.release());
    layout->addWidget(userAgentHint);
    layout->addWidget(remarks.release());
    layout->addSpacing(4);
    layout->addWidget(actionRow);

    const auto submit = [this](Ui::InputField* field) {
        field->submits().start(
            [this](Qt::KeyboardModifiers) { acceptChanges(); },
            [](auto&&) {},
            [] {},
            field->lifetime());
    };
    submit(m_nameEdit);
    submit(m_urlEdit);
    submit(m_userAgentEdit);

    m_nameEdit->setFocusFast();
    resize(kDialogWidth, sizeHint().height());
}

bool SubscriptionDialog::editSubscription(QWidget* parent, Subscription& subscription)
{
    SubscriptionDialog dialog(parent, subscription);
    return dialog.exec() == QDialog::Accepted;
}

bool SubscriptionDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_enabledCheck && event->type() == QEvent::KeyPress) {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            acceptChanges();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void SubscriptionDialog::acceptChanges()
{
    m_subscription.name = m_nameEdit->getLastText().trimmed();
    m_subscription.url = m_urlEdit->getLastText().trimmed();
    m_subscription.enabled = m_enabledCheck->checked();
    m_subscription.userAgent = m_userAgentEdit->getLastText().trimmed();
    m_subscription.remarks = m_remarksEdit->getLastText().trimmed();
    if (!m_subscription.enabled) {
        m_subscription.lastStatus = SubscriptionStatus::Disabled;
    }
    QDialog::accept();
}

} // namespace zarya
