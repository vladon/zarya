#include "ui/desktopapp/LibUiSpikeDialog.h"

#include "base/object_ptr.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "ui/qt_object_factory.h"
#include "ui/rp_widget.h"
#include "ui/style/style_core.h"
#include "ui/style/style_core_font.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"

#include <QVBoxLayout>
#include <rpl/rpl.h>

namespace zarya {

LibUiSpikeDialog::LibUiSpikeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Desktop App UI spike"));
    resize(420, 260);

    style::internal::StartFonts();
    style::StartManager(style::Scale());

    auto* root = new Ui::RpWidget(this);
    auto* layout = Ui::CreateChild<Ui::VerticalLayout>(root);

    layout->add(
        object_ptr<Ui::FlatLabel>(
            layout,
            tr("This dialog uses desktop-app/lib_ui (FlatLabel + RoundButton). "
               "The rest of Zarya still uses Qt Widgets + ThemeManager."),
            st::boxLabel),
        st::boxRowPadding);

    const auto closeButton = layout->add(
        object_ptr<Ui::RoundButton>(
            layout,
            rpl::single(tr("Close")),
            st::defaultBoxButton),
        st::boxRowPadding);
    closeButton->setClickedCallback([this] { accept(); });

    root->sizeValue() | rpl::on_next(
        [=](QSize size) {
            layout->resizeToWidth(size.width());
            layout->move(0, 0);
        },
        root->lifetime());

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(root);
}

} // namespace zarya
