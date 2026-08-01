#pragma once

#include "base/object_ptr.h"

#include <QString>

class QWidget;

namespace Ui {
class DropdownMenu;
class RoundButton;
} // namespace Ui

namespace zarya {

enum class ZaryaButtonRole {
    Primary,
    Secondary,
    Destructive,
};

[[nodiscard]] object_ptr<Ui::RoundButton> makeZaryaButton(
    QWidget* parent,
    const QString& text,
    ZaryaButtonRole role);

void toggleZaryaDropdownMenu(Ui::DropdownMenu* menu, QWidget* anchor);

} // namespace zarya
