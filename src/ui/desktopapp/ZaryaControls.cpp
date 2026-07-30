#include "ui/desktopapp/ZaryaControls.h"

#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "ui/widgets/buttons.h"

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

} // namespace zarya
