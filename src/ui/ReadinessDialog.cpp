#include "ui/ReadinessDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace zarya {

ReadinessDialog::ReadinessDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Zarya Setup"));
    resize(440, 260);

    auto* text = new QLabel(
        tr("Zarya is not fully configured yet.\n\n"
           "Recommended next steps:\n"
           "1. Install Xray in Core Manager\n"
           "2. Import or add a profile\n"
           "3. Choose routing profile\n"
           "4. Start profile"),
        this);
    text->setWordWrap(true);

    auto* coreButton = new QPushButton(tr("Open Core Manager"), this);
    auto* importButton = new QPushButton(tr("Import Profile"), this);
    auto* settingsButton = new QPushButton(tr("Open Settings"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);

    connect(coreButton, &QPushButton::clicked, this, [this]() {
        emit openCoreManagerRequested();
        accept();
    });
    connect(importButton, &QPushButton::clicked, this, [this]() {
        emit importProfileRequested();
        accept();
    });
    connect(settingsButton, &QPushButton::clicked, this, [this]() {
        emit openSettingsRequested();
        accept();
    });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(text);
    layout->addWidget(coreButton);
    layout->addWidget(importButton);
    layout->addWidget(settingsButton);
    layout->addStretch();
    layout->addWidget(closeButton);
}

} // namespace zarya
