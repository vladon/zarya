#include "ui/desktopapp/ProfileActionStrip.h"

#include "ui/desktopapp/ZaryaControls.h"
#include "ui/desktopapp/ZaryaSelector.h"

#include "ui/widgets/buttons.h"
#include "ui/widgets/dropdown_menu.h"

#include <QAction>
#include <QHBoxLayout>
#include <QMetaObject>
#include <QResizeEvent>
#include <array>
#include <utility>

namespace zarya {
namespace {

QString displayText(QString text)
{
    constexpr QChar escapedAmpersand(0xF000);
    text.replace(QStringLiteral("&&"), QString(escapedAmpersand));
    text.remove(QLatin1Char('&'));
    text.replace(escapedAmpersand, QLatin1Char('&'));
    return text;
}

} // namespace

ProfileActionStrip::ProfileActionStrip(
    ProfileActionStripActions actions,
    const QString& moreText,
    QWidget* parent)
    : QWidget(parent)
    , m_actions(std::move(actions))
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(8);

    const auto addActionButton = [this](QAction* action, ZaryaButtonRole role) {
        auto button = makeZaryaButton(this, displayText(action->text()), role);
        QWidget* result = button.data();
        m_layout->addWidget(button.release());
        bindButton(result, action);
        return result;
    };

    addActionButton(m_actions.add, ZaryaButtonRole::Secondary);
    m_importButton = addActionButton(m_actions.importProfiles, ZaryaButtonRole::Secondary);
    m_subscriptionsButton = addActionButton(m_actions.subscriptions, ZaryaButtonRole::Secondary);

    m_profileSelector = new ZaryaSelector(this);
    m_profileSelector->setMinimumWidth(180);
    m_layout->addWidget(m_profileSelector);

    m_testButton = addActionButton(m_actions.testSelected, ZaryaButtonRole::Secondary);
    m_layout->addStretch();
    addActionButton(m_actions.start, ZaryaButtonRole::Primary);
    addActionButton(m_actions.stop, ZaryaButtonRole::Destructive);

    auto moreButton = makeZaryaButton(this, moreText, ZaryaButtonRole::Secondary);
    m_moreButton = moreButton.data();
    m_layout->addWidget(moreButton.release());

    auto* menu = new Ui::DropdownMenu(window());
    menu->setAutoHiding(false);
    m_menu = menu;
    static_cast<Ui::RoundButton*>(m_moreButton)->setClickedCallback([this] {
        rebuildOverflowMenu();
        toggleZaryaDropdownMenu(
            static_cast<Ui::DropdownMenu*>(m_menu.data()),
            static_cast<Ui::RoundButton*>(m_moreButton));
    });
}

ProfileActionStrip::~ProfileActionStrip()
{
    delete m_menu.data();
}

ZaryaSelector* ProfileActionStrip::profileSelector() const
{
    return m_profileSelector;
}

void ProfileActionStrip::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateResponsiveVisibility();
}

void ProfileActionStrip::bindButton(QWidget* buttonWidget, QAction* action)
{
    auto* button = static_cast<Ui::RoundButton*>(buttonWidget);
    const auto sync = [button, action] {
        const QString text = displayText(action->text());
        button->setText(rpl::single(text));
        button->setAccessibleName(text);
        button->setDisabled(!action->isEnabled());
        button->setVisible(action->isVisible());
    };
    sync();
    connect(action, &QAction::changed, this, sync);
    button->setClickedCallback([action] {
        QMetaObject::invokeMethod(action, [action] { action->trigger(); }, Qt::QueuedConnection);
    });
}

void ProfileActionStrip::rebuildOverflowMenu()
{
    auto* menu = static_cast<Ui::DropdownMenu*>(m_menu.data());
    menu->clearActions();
    bool hasAction = false;
    bool separatorPending = false;
    for (QAction* source : m_actions.overflow) {
        if (!source) {
            separatorPending = hasAction;
            continue;
        }
        if (!source->isVisible()) {
            continue;
        }
        if (separatorPending) {
            menu->addSeparator();
            separatorPending = false;
        }
        auto action = menu->addAction(displayText(source->text()), [source] { source->trigger(); });
        action->setEnabled(source->isEnabled());
        hasAction = true;
    }
    menu->resizeToContent();
}

void ProfileActionStrip::updateResponsiveVisibility()
{
    const std::array optionalButtons = {
        m_testButton,
        m_subscriptionsButton,
        m_importButton,
    };
    for (QWidget* button : optionalButtons) {
        button->show();
    }
    m_layout->activate();
    for (QWidget* button : optionalButtons) {
        if (m_layout->sizeHint().width() <= width()) {
            break;
        }
        button->hide();
        m_layout->activate();
    }
}

} // namespace zarya
