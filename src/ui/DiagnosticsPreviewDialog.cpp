#include "ui/DiagnosticsPreviewDialog.h"

#include "ui/desktopapp/ZaryaFormControls.h"

#include <QTimer>
#include <QVBoxLayout>

namespace zarya {

DiagnosticsPreviewDialog::DiagnosticsPreviewDialog(const DiagnosticsPreviewResult& preview,
                                                 QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Diagnostics Preview"));
    resize(560, 480);

    m_redactionLabel = new ZaryaBodyText({}, this);
    m_redactionLabel->setText(
        tr("Redaction: %1\nSecrets included: %2")
            .arg(preview.redactionMode, preview.secretsIncluded ? tr("yes")
                                                                 : tr("no")));

    m_warningsLabel = new ZaryaBodyText({}, this);
    if (preview.warnings.isEmpty()) {
        m_warningsLabel->setText(tr("Warnings: none"));
    } else {
        m_warningsLabel->setText(tr("Warnings:\n") + preview.warnings.join(QStringLiteral("\n")));
    }

    m_filesList = new ZaryaTextArea({}, this, 260);
    m_filesList->setReadOnly(true);
    m_filesList->setAccessibleLabel(tr("Included files"));
    m_filesList->setText(preview.files.join(QStringLiteral("\n")));

    auto* closeButton = new ZaryaActionButton(tr("Close"), this);
    connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::accept);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(new ZaryaBodyText(tr("Included files:"), this));
    layout->addWidget(m_filesList, 1);
    layout->addWidget(m_redactionLabel);
    layout->addWidget(m_warningsLabel);
    layout->addWidget(closeButton);
    QWidget::setTabOrder(m_filesList, closeButton);
}

void DiagnosticsPreviewDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, m_filesList, [this] {
        m_filesList->setFocus(Qt::OtherFocusReason);
    });
}

} // namespace zarya
