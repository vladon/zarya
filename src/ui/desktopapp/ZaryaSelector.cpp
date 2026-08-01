#include "ui/desktopapp/ZaryaSelector.h"

#include "ui/desktopapp/ZaryaControls.h"

#include "ui/widgets/buttons.h"
#include "ui/widgets/dropdown_menu.h"

#include <QAction>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <algorithm>
#include <utility>

namespace zarya {

ZaryaSelector::ZaryaSelector(QWidget* parent)
    : QWidget(parent)
{
    auto button = makeZaryaButton(this, QString(), ZaryaButtonRole::Secondary);
    m_button = button.data();
    static_cast<Ui::RoundButton*>(m_button)->setClickedCallback([this] { toggleMenu(); });

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(button.release());

    auto* menu = new Ui::DropdownMenu(window());
    menu->setAutoHiding(false);
    m_menu = menu;
}

ZaryaSelector::~ZaryaSelector()
{
    delete m_menu.data();
}

void ZaryaSelector::setItems(QVector<ZaryaSelectorItem> items, const QString& currentKey)
{
    m_items = std::move(items);

    auto* menu = static_cast<Ui::DropdownMenu*>(m_menu.data());
    menu->clearActions();
    for (const ZaryaSelectorItem& item : m_items) {
        const QString key = item.key;
        auto action = menu->addAction(item.text, [this, key] {
            if (setCurrentKey(key)) {
                Q_EMIT currentKeyChanged(m_currentKey);
            }
        });
        action->setEnabled(item.enabled);
    }
    menu->resizeToContent();

    const auto requested = std::find_if(
        m_items.cbegin(),
        m_items.cend(),
        [&currentKey](const ZaryaSelectorItem& item) {
            return item.enabled && item.key == currentKey;
        });
    const auto selected = (requested != m_items.cend())
        ? requested
        : std::find_if(
            m_items.cbegin(),
            m_items.cend(),
            [](const ZaryaSelectorItem& item) { return item.enabled; });
    m_currentKey = (selected != m_items.cend()) ? selected->key : QString();
    applyCurrentItem();
}

bool ZaryaSelector::setCurrentKey(const QString& key)
{
    const auto found = std::find_if(
        m_items.cbegin(),
        m_items.cend(),
        [&key](const ZaryaSelectorItem& item) { return item.enabled && item.key == key; });
    if (found == m_items.cend()) {
        return false;
    }
    if (m_currentKey == key) {
        applyCurrentItem();
        return false;
    }
    m_currentKey = key;
    applyCurrentItem();
    return true;
}

QString ZaryaSelector::currentKey() const
{
    return m_currentKey;
}

void ZaryaSelector::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    static_cast<Ui::RoundButton*>(m_button)->setFullWidth(width());
}

void ZaryaSelector::applyCurrentItem()
{
    const auto found = std::find_if(
        m_items.cbegin(),
        m_items.cend(),
        [this](const ZaryaSelectorItem& item) { return item.key == m_currentKey; });
    const QString text = (found != m_items.cend()) ? found->text : QString();
    auto* button = static_cast<Ui::RoundButton*>(m_button);
    button->setText(rpl::single(text));
    button->setAccessibleName(text);
}

void ZaryaSelector::toggleMenu()
{
    toggleZaryaDropdownMenu(
        static_cast<Ui::DropdownMenu*>(m_menu.data()),
        static_cast<Ui::RoundButton*>(m_button));
}

} // namespace zarya
