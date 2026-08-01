#include "ui/desktopapp/ZaryaControls.h"

#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/dropdown_menu.h"

#include <algorithm>
#include <rpl/rpl.h>

namespace zarya {

object_ptr<Ui::RoundButton> makeZaryaButton(
    QWidget* parent,
    const QString& text,
    ZaryaButtonRole role)
{
    const style::RoundButton* buttonStyle = nullptr;
    switch (role) {
    case ZaryaButtonRole::Primary:
        buttonStyle = &st::defaultActiveButton;
        break;
    case ZaryaButtonRole::Secondary:
        buttonStyle = &st::defaultBoxButton;
        break;
    case ZaryaButtonRole::Destructive:
        buttonStyle = &st::attentionBoxButton;
        break;
    }

    auto button = object_ptr<Ui::RoundButton>(
        parent,
        rpl::single(text),
        *buttonStyle);
    button->setAccessibleName(text);
    return button;
}

void toggleZaryaDropdownMenu(Ui::DropdownMenu* menu, QWidget* anchor)
{
    if (!menu->isHidden()) {
        menu->hideAnimated();
        return;
    }

    menu->resizeToContent();
    QWidget* menuParent = menu->parentWidget();
    const QPoint below = anchor->mapTo(menuParent, QPoint(0, anchor->height()));
    const int maximumX = std::max(menuParent->width() - menu->width(), 0);
    const int x = std::clamp(below.x(), 0, maximumX);
    const bool fitsBelow = below.y() + menu->height() <= menuParent->height();
    const int y = fitsBelow
        ? below.y()
        : std::max(below.y() - anchor->height() - menu->height(), 0);
    menu->move(x, y);
    menu->raise();
    menu->showAnimated(
        fitsBelow
            ? Ui::PanelAnimation::Origin::TopLeft
            : Ui::PanelAnimation::Origin::BottomLeft);
}

} // namespace zarya
