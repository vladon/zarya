#include "ui/DiagnosticsDialog.h"

#include "diagnostics/DiagnosticsCategory.h"
#include "packaging/PublicBetaDocs.h"
#include "ui/DiagnosticsPreviewDialog.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
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

    m_redactionGroup = new ZaryaRadioGroup(0, this);
    m_redactionGroup->addOption(
        0, tr("Strict — redact credentials, hosts, URLs, usernames in paths"));
    m_redactionGroup->addOption(
        1, tr("Basic — redact credentials but keep hosts and ports"));
    auto* redactionSection = new ZaryaFormSection(tr("Redaction"), this);
    redactionSection->addWidget(m_redactionGroup);

    m_runValidationCheck = new ZaryaCheckBox(
        tr("Run config validation while creating diagnostics"), this, true);
    m_extendedLogsCheck = new ZaryaCheckBox(tr("Extended logs (5000 lines)"), this);
    m_machinePathsCheck = new ZaryaCheckBox(
        tr("Include machine paths (still redacted usernames in strict mode)"), this);

    auto* includeSection = new ZaryaFormSection(tr("Include"), this);
    includeSection->addWidget(new ZaryaBodyText(
        tr("App/platform, cores, runtime, helper, proxy, kill switch, routing/DNS, geo, rule-sets, validation, config previews, logs"),
        this));
    includeSection->addWidget(m_runValidationCheck);
    includeSection->addWidget(m_extendedLogsCheck);
    includeSection->addWidget(m_machinePathsCheck);

    const QString defaultName =
        QStringLiteral("zarya-diagnostics-%1.zarya-diagnostics.zip")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    m_outputEdit = new ZaryaTextField(tr("Output"), this);
    m_outputEdit->setText(defaultName);
    auto* browseButton = new ZaryaActionButton(tr("Browse…"), this);
    connect(browseButton, &ZaryaActionButton::clicked,
            this, &DiagnosticsDialog::onBrowse);

    auto* outputRow = new QHBoxLayout;
    outputRow->addWidget(m_outputEdit, 1);
    outputRow->addWidget(browseButton);

    auto* previewButton = new ZaryaActionButton(tr("Preview"), this);
    m_createButton = new ZaryaActionButton(tr("Create Bundle"), this);
    auto* cancelButton = new ZaryaActionButton(tr("Cancel"), this);
    connect(previewButton, &ZaryaActionButton::clicked,
            this, &DiagnosticsDialog::onPreview);
    connect(m_createButton, &ZaryaActionButton::clicked,
            this, &DiagnosticsDialog::onCreate);
    connect(cancelButton, &ZaryaActionButton::clicked, this, &QDialog::reject);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(previewButton);
    buttons->addStretch();
    buttons->addWidget(m_createButton);
    buttons->addWidget(cancelButton);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(redactionSection);
    layout->addWidget(includeSection);
    layout->addWidget(new ZaryaBodyText(tr("Output"), this));
    layout->addLayout(outputRow);
    layout->addLayout(buttons);
}

DiagnosticsOptions DiagnosticsDialog::buildOptions() const
{
    DiagnosticsOptions options;
    options.redactionMode = m_redactionGroup->value() == 0
        ? DiagnosticsRedactionMode::Strict
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
        UiMessagePresenter::error(this, tr("Diagnostics Bundle"), error);
        return;
    }
    m_createButton->setEnabled(true);

    if (m_logCallback) {
        m_logCallback(tr("Diagnostics bundle created: %1").arg(outputPath));
    }

    const QString action = UiMessagePresenter::choose(
        this,
        tr("Diagnostics Bundle"),
        tr("Diagnostics bundle created:\n%1").arg(outputPath)
            + QStringLiteral("\n\n")
            + tr("This bundle is redacted, but review the diagnostics archive before sharing it "
                 "publicly."),
        UiMessageTone::Information,
        {
            {QStringLiteral("open-folder"), tr("Open Folder")},
            {QStringLiteral("issue-template"), tr("Open Issue Template")},
            {QStringLiteral("close"), tr("Close"), UiMessageActionRole::Primary, true, true},
        });

    if (action == QStringLiteral("open-folder")) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(outputPath).absolutePath()));
    } else if (action == QStringLiteral("issue-template")) {
        if (!PublicBetaDocs::openIssueReporting()) {
            UiMessagePresenter::warning(
                this, tr("Diagnostics Bundle"),
                tr("Issue reporting instructions are not bundled with this build."));
        }
    }
    accept();
}

} // namespace zarya
