#include "ui/desktopapp/LibUiSpikeDialog.h"

#include "base/object_ptr.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "ui/qt_object_factory.h"
#include "ui/rp_widget.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"

#include <QMetaObject>
#include <QVBoxLayout>
#include <rpl/rpl.h>

namespace zarya {

LibUiSpikeDialog::LibUiSpikeDialog(QWidget* parent)
    : QDialog(parent)
{
    // Spike is English-only (dev surface); do not use tr().
    setWindowTitle(QStringLiteral("Desktop App UI spike"));
    resize(420, 260);

    auto* root = new Ui::RpWidget(this);
    auto* layout = Ui::CreateChild<Ui::VerticalLayout>(root);

    layout->add(
        object_ptr<Ui::FlatLabel>(
            layout,
            QStringLiteral(
                "This dialog uses desktop-app/lib_ui (FlatLabel + RoundButton). "
                "The rest of Zarya still uses Qt Widgets + ThemeManager."),
            st::boxLabel),
        st::boxRowPadding);

    const auto closeButton = layout->add(
        object_ptr<Ui::RoundButton>(
            layout,
            rpl::single(QStringLiteral("Close")),
            st::defaultBoxButton),
        st::boxRowPadding);
    // Defer accept(): RoundButton click runs inside mouseReleaseEvent / ripple
    // animation; destroying the dialog synchronously Asserts or UAF.
    closeButton->setClickedCallback([this] {
        QMetaObject::invokeMethod(
            this,
            [this] { accept(); },
            Qt::QueuedConnection);
    });

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
