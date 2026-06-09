#include "ui/DiagnosticsDialog.h"

#include "diagnostics/DiagnosticsCategory.h"
#include "ui/DiagnosticsPreviewDialog.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QUrl>
#include <QVBoxLayout>

namespace zarya {

DiagnosticsDialog::DiagnosticsDialog(DiagnosticsManager& manager,
                                     const std::function<void(const QString&)>& logCallback,
                                     QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_logCallback(logCallback)
{
    setWindowTitle(tr("Create Diagnostics Bundle"));
    resize(560, 420);

    m_strictRedactionRadio = new QRadioButton(tr("Strict — redact credentials, hosts, URLs, usernames in paths"), this);
    m_basicRedactionRadio = new QRadioButton(tr("Basic — redact credentials but keep hosts and ports"), this);
    m_strictRedactionRadio->setChecked(true);

    auto* redactionGroup = new QGroupBox(tr("Redaction"), this);
    auto* redactionLayout = new QVBoxLayout(redactionGroup);
    redactionLayout->addWidget(m_strictRedactionRadio);
    redactionLayout->addWidget(m_basicRedactionRadio);

    m_runValidationCheck = new QCheckBox(tr("Run config validation while creating diagnostics"), this);
    m_runValidationCheck->setChecked(true);
    m_extendedLogsCheck = new QCheckBox(tr("Extended logs (5000 lines)"), this);
    m_machinePathsCheck = new QCheckBox(tr("Include machine paths (still redacted usernames in strict mode)"), this);

    auto* includeGroup = new QGroupBox(tr("Include"), this);
    auto* includeLayout = new QVBoxLayout(includeGroup);
    includeLayout->addWidget(new QLabel(tr("App/platform, cores, runtime, helper, proxy, kill switch, routing/DNS, geo, rule-sets, validation, config previews, logs"), this));
    includeLayout->addWidget(m_runValidationCheck);
    includeLayout->addWidget(m_extendedLogsCheck);
    includeLayout->addWidget(m_machinePathsCheck);

    const QString defaultName =
        QStringLiteral("zarya-diagnostics-%1.zarya-diagnostics.zip")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    m_outputEdit = new QLineEdit(defaultName, this);
    auto* browseButton = new QPushButton(tr("Browse…"), this);
    connect(browseButton, &QPushButton::clicked, this, &DiagnosticsDialog::onBrowse);

    auto* outputRow = new QHBoxLayout;
    outputRow->addWidget(m_outputEdit, 1);
    outputRow->addWidget(browseButton);

    auto* previewButton = new QPushButton(tr("Preview"), this);
    m_createButton = new QPushButton(tr("Create Bundle"), this);
    auto* cancelButton = new QPushButton(tr("Cancel"), this);
    connect(previewButton, &QPushButton::clicked, this, &DiagnosticsDialog::onPreview);
    connect(m_createButton, &QPushButton::clicked, this, &DiagnosticsDialog::onCreate);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(previewButton);
    buttons->addStretch();
    buttons->addWidget(m_createButton);
    buttons->addWidget(cancelButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(redactionGroup);
    layout->addWidget(includeGroup);
    layout->addWidget(new QLabel(tr("Output"), this));
    layout->addLayout(outputRow);
    layout->addLayout(buttons);
}

DiagnosticsOptions DiagnosticsDialog::buildOptions() const
{
    DiagnosticsOptions options;
    options.redactionMode = m_strictRedactionRadio->isChecked() ? DiagnosticsRedactionMode::Strict
                                                                : DiagnosticsRedactionMode::Basic;
    for (DiagnosticsCategory category : defaultDiagnosticsCategories()) {
        options.categories.insert(category);
    }
    options.runConfigValidation = m_runValidationCheck->isChecked();
    options.extendedLogs = m_extendedLogsCheck->isChecked();
    options.includeMachinePaths = m_machinePathsCheck->isChecked();
    options.outputPath = m_outputEdit->text().trimmed();
    return options;
}

void DiagnosticsDialog::onBrowse()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Diagnostics Bundle"), m_outputEdit->text(),
        tr("Zarya Diagnostics (*.zarya-diagnostics.zip)"));
    if (!path.isEmpty()) {
        m_outputEdit->setText(path);
    }
}

void DiagnosticsDialog::onPreview()
{
    const DiagnosticsPreviewResult preview = m_manager.buildPreview(buildOptions());
    DiagnosticsPreviewDialog dialog(preview, this);
    dialog.exec();
}

void DiagnosticsDialog::onCreate()
{
    const DiagnosticsOptions options = buildOptions();
    QString outputPath;
    QString error;
    m_createButton->setEnabled(false);
    if (!m_manager.createBundle(options, &outputPath, &error)) {
        m_createButton->setEnabled(true);
        QMessageBox::critical(this, tr("Diagnostics Bundle"), error);
        return;
    }
    m_createButton->setEnabled(true);

    if (m_logCallback) {
        m_logCallback(tr("Diagnostics bundle created: %1").arg(outputPath));
    }

    const auto answer = QMessageBox::information(
        this, tr("Diagnostics Bundle"),
        tr("Diagnostics bundle created:\n%1\n\nThis bundle is redacted, but review it "
           "before sharing.")
            .arg(outputPath),
        QMessageBox::Open | QMessageBox::Close, QMessageBox::Close);

    if (answer == QMessageBox::Open) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(outputPath).absolutePath()));
    }
    accept();
}

} // namespace zarya
