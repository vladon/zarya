#include "ui/SingBoxConfigPreviewDialog.h"

#include "core/CoreManager.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace zarya {

SingBoxConfigPreviewDialog::SingBoxConfigPreviewDialog(const QString& jsonText,
                                                       const QStringList& warnings,
                                                       CoreManager* coreManager, QWidget* parent)
    : QDialog(parent)
    , m_jsonText(jsonText)
    , m_coreManager(coreManager)
{
    setWindowTitle(tr("sing-box TUN config preview"));
    resize(720, 560);

    auto* warningsSection = new ZaryaFormSection(tr("Warnings"), this);
    m_warningsView = new ZaryaTextArea({}, this, 100);
    m_warningsView->setReadOnly(true);
    m_warningsView->setAccessibleLabel(tr("Warnings"));
    m_warningsView->setText(warnings.isEmpty() ? tr("(none)")
                                                : warnings.join(QStringLiteral("\n")));
    warningsSection->addWidget(m_warningsView);

    auto* jsonSection = new ZaryaFormSection(tr("Generated JSON"), this);
    m_editor = new ZaryaTextArea({}, this, 300);
    m_editor->setReadOnly(true);
    m_editor->setAccessibleLabel(tr("Generated JSON"));
    m_editor->setText(jsonText);
    jsonSection->addWidget(m_editor);

    auto* copyButton = new ZaryaActionButton(tr("Copy"), this);
    connect(copyButton, &ZaryaActionButton::clicked, this,
            &SingBoxConfigPreviewDialog::onCopy);

    auto* saveButton = new ZaryaActionButton(tr("Save As…"), this);
    connect(saveButton, &ZaryaActionButton::clicked, this,
            &SingBoxConfigPreviewDialog::onSaveAs);

    auto* checkButton = new ZaryaActionButton(tr("Run sing-box check"), this);
    connect(checkButton, &ZaryaActionButton::clicked, this,
            &SingBoxConfigPreviewDialog::onRunCheck);

    auto* actionRow = new QHBoxLayout;
    actionRow->addWidget(copyButton);
    actionRow->addWidget(saveButton);
    actionRow->addWidget(checkButton);
    actionRow->addStretch();

    auto* closeButton = new ZaryaActionButton(tr("Close"), this);
    connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(warningsSection);
    layout->addWidget(jsonSection, 1);
    layout->addLayout(actionRow);
    layout->addWidget(closeButton);
    (warnings.isEmpty() ? m_editor : m_warningsView)->setFocus(Qt::OtherFocusReason);
}

void SingBoxConfigPreviewDialog::onCopy()
{
    QApplication::clipboard()->setText(m_jsonText);
}

void SingBoxConfigPreviewDialog::onSaveAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save sing-box config"), AppPaths::singBoxTunConfigPath(),
        tr("JSON (*.json);;All files (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        UiMessagePresenter::error(this, tr("Save failed"), file.errorString());
        return;
    }
    file.write(m_jsonText.toUtf8());
}

void SingBoxConfigPreviewDialog::onRunCheck()
{
    UiMessagePresenter::information(this, tr("sing-box check"),
                                    tr("Embedded validation is performed when the TUN runtime starts."));
}

} // namespace zarya
