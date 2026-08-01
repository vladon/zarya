#include "ui/desktopapp/ProfileActionStrip.h"

#include "ui/desktopapp/ZaryaControls.h"
#include "ui/desktopapp/ZaryaSelector.h"

#include "ui/widgets/buttons.h"
#include "ui/widgets/dropdown_menu.h"

#include <QAction>
#include <QHBoxLayout>
#include <QMetaObject>
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
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    const auto addActionButton = [this, layout](QAction* action, ZaryaButtonRole role) {
        auto button = makeZaryaButton(this, displayText(action->text()), role);
        QWidget* result = button.data();
        layout->addWidget(button.release());
        bindButton(result, action);
        return result;
    };

    addActionButton(m_actions.add, ZaryaButtonRole::Secondary);
    addActionButton(m_actions.importProfiles, ZaryaButtonRole::Secondary);
    addActionButton(m_actions.subscriptions, ZaryaButtonRole::Secondary);

    m_profileSelector = new ZaryaSelector(this);
    m_profileSelector->setMinimumWidth(180);
    layout->addWidget(m_profileSelector);

    addActionButton(m_actions.testSelected, ZaryaButtonRole::Secondary);
    m_startButton = addActionButton(m_actions.start, ZaryaButtonRole::Primary);
    m_stopButton = addActionButton(m_actions.stop, ZaryaButtonRole::Primary);

    auto moreButton = makeZaryaButton(this, moreText, ZaryaButtonRole::Secondary);
    m_moreButton = moreButton.data();
    layout->addWidget(moreButton.release());
    layout->addStretch();

    auto* menu = new Ui::DropdownMenu(window());
    menu->setAutoHiding(false);
    m_menu = menu;
    static_cast<Ui::RoundButton*>(m_moreButton)->setClickedCallback([this] {
        rebuildOverflowMenu();
        toggleZaryaDropdownMenu(
            static_cast<Ui::DropdownMenu*>(m_menu),
            static_cast<Ui::RoundButton*>(m_moreButton));
    });
}

ProfileActionStrip::~ProfileActionStrip()
{
    delete m_menu;
}

ZaryaSelector* ProfileActionStrip::profileSelector() const
{
    return m_profileSelector;
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
    auto* menu = static_cast<Ui::DropdownMenu*>(m_menu);
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

} // namespace zarya
