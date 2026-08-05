#include "ui/ReadinessDialog.h"

#include "ui/desktopapp/ZaryaFormControls.h"

#include <QTimer>
#include <QVBoxLayout>

namespace zarya {

ReadinessDialog::ReadinessDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Zarya Setup"));
    resize(440, 260);

    auto* text = new ZaryaBodyText(
        tr("Zarya is not fully configured yet.\n\n"
           "Recommended next steps:\n"
           "1. Confirm Xray (bundled in release builds, or install via Core Manager)\n"
           "2. Import or add a profile\n"
           "3. Choose routing profile\n"
           "4. Start profile"),
        this);
    auto* coreButton = new ZaryaActionButton(tr("Open Core Manager"), this);
    auto* importButton = new ZaryaActionButton(tr("Import Profile"), this);
    auto* settingsButton = new ZaryaActionButton(tr("Open Settings"), this);
    auto* closeButton = new ZaryaActionButton(tr("Close"), this);

    connect(coreButton, &ZaryaActionButton::clicked, this, [this]() {
        emit openCoreManagerRequested();
        accept();
    });
    connect(importButton, &ZaryaActionButton::clicked, this, [this]() {
        emit importProfileRequested();
        accept();
    });
    connect(settingsButton, &ZaryaActionButton::clicked, this, [this]() {
        emit openSettingsRequested();
        accept();
    });
    connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::accept);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(text);
    layout->addWidget(coreButton);
    layout->addWidget(importButton);
    layout->addWidget(settingsButton);
    layout->addStretch();
    layout->addWidget(closeButton);

    QTimer::singleShot(0, coreButton, [coreButton] {
        coreButton->focusProxy()->setFocus(Qt::OtherFocusReason);
    });
}

} // namespace zarya
