#include "ui/desktopapp/ZaryaControls.h"

#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/fields/input_field.h"

#include <rpl/rpl.h>

namespace zarya {

object_ptr<Ui::InputField> makeZaryaInputField(
    QWidget* parent,
    const QString& label,
    const QString& value,
    ZaryaInputMode mode)
{
    const auto inputMode = mode == ZaryaInputMode::MultiLine
        ? Ui::InputField::Mode::MultiLine
        : Ui::InputField::Mode::SingleLine;
    auto field = object_ptr<Ui::InputField>(
        parent,
        st::defaultInputField,
        inputMode,
        rpl::single(label),
        value);
    field->setAccessibleName(label);
    field->setSubmitSettings(
        mode == ZaryaInputMode::MultiLine
            ? Ui::InputField::SubmitSettings::None
            : Ui::InputField::SubmitSettings::Enter);
    return field;
}

object_ptr<Ui::Checkbox> makeZaryaCheckbox(
    QWidget* parent,
    const QString& text,
    bool checked)
{
    auto checkbox = object_ptr<Ui::Checkbox>(parent, text, checked);
    checkbox->setAccessibleName(text);
    checkbox->setFocusPolicy(Qt::StrongFocus);
    return checkbox;
}

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
