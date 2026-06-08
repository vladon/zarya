#include "ui/DiagnosticsPreviewDialog.h"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace zarya {

DiagnosticsPreviewDialog::DiagnosticsPreviewDialog(const DiagnosticsPreviewResult& preview,
                                                 QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Diagnostics Preview"));
    resize(560, 480);

    m_redactionLabel = new QLabel(this);
    m_redactionLabel->setText(
        QStringLiteral("Redaction: %1\nSecrets included: %2")
            .arg(preview.redactionMode, preview.secretsIncluded ? QStringLiteral("yes")
                                                                 : QStringLiteral("no")));

    m_warningsLabel = new QLabel(this);
    m_warningsLabel->setWordWrap(true);
    if (preview.warnings.isEmpty()) {
        m_warningsLabel->setText(QStringLiteral("Warnings: none"));
    } else {
        m_warningsLabel->setText(QStringLiteral("Warnings:\n") + preview.warnings.join(QStringLiteral("\n")));
        m_warningsLabel->setStyleSheet(QStringLiteral("color: #b45309;"));
    }

    m_filesList = new QListWidget(this);
    for (const QString& file : preview.files) {
        m_filesList->addItem(file);
    }

    auto* closeButton = new QPushButton(QStringLiteral("Close"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("Included files:"), this));
    layout->addWidget(m_filesList, 1);
    layout->addWidget(m_redactionLabel);
    layout->addWidget(m_warningsLabel);
    layout->addWidget(closeButton);
}

} // namespace zarya
