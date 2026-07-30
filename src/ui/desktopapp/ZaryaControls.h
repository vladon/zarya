#pragma once

#include "base/algorithm.h"
#include "base/basic_types.h"
#include "base/object_ptr.h"

#include <QString>

class QWidget;

namespace Ui {
class Checkbox;
class InputField;
class RoundButton;
} // namespace Ui

namespace zarya {

enum class ZaryaInputMode {
    SingleLine,
    MultiLine,
};

enum class ZaryaButtonRole {
    Primary,
    Secondary,
    Destructive,
};

[[nodiscard]] object_ptr<Ui::InputField> makeZaryaInputField(
    QWidget* parent,
    const QString& label,
    const QString& value,
    ZaryaInputMode mode = ZaryaInputMode::SingleLine);

[[nodiscard]] object_ptr<Ui::Checkbox> makeZaryaCheckbox(
    QWidget* parent,
    const QString& text,
    bool checked);

[[nodiscard]] object_ptr<Ui::RoundButton> makeZaryaButton(
    QWidget* parent,
    const QString& text,
    ZaryaButtonRole role);

} // namespace zarya
