#include "ui/SingBoxConfigPreviewDialog.h"

#include "core/CoreManager.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace zarya {

SingBoxConfigPreviewDialog::SingBoxConfigPreviewDialog(const QString& jsonText,
                                                       const QStringList& warnings,
                                                       CoreManager* coreManager, QWidget* parent)
    : QDialog(parent)
    , m_jsonText(jsonText)
    , m_coreManager(coreManager)
{
    setWindowTitle(QStringLiteral("sing-box TUN config preview"));
    resize(720, 560);

    auto* warningsLabel = new QLabel(QStringLiteral("Warnings"), this);
    m_warningsView = new QPlainTextEdit(this);
    m_warningsView->setReadOnly(true);
    m_warningsView->setMaximumHeight(120);
    m_warningsView->setPlainText(warnings.isEmpty()
                                     ? QStringLiteral("(none)")
                                     : warnings.join(QStringLiteral("\n")));

    auto* jsonLabel = new QLabel(QStringLiteral("Generated JSON"), this);
    m_editor = new QPlainTextEdit(this);
    m_editor->setReadOnly(true);
    m_editor->setPlainText(jsonText);

    auto* copyButton = new QPushButton(QStringLiteral("Copy"), this);
    connect(copyButton, &QPushButton::clicked, this, &SingBoxConfigPreviewDialog::onCopy);

    auto* saveButton = new QPushButton(QStringLiteral("Save As…"), this);
    connect(saveButton, &QPushButton::clicked, this, &SingBoxConfigPreviewDialog::onSaveAs);

    auto* checkButton = new QPushButton(QStringLiteral("Run sing-box check"), this);
    connect(checkButton, &QPushButton::clicked, this, &SingBoxConfigPreviewDialog::onRunCheck);

    auto* actionRow = new QHBoxLayout;
    actionRow->addWidget(copyButton);
    actionRow->addWidget(saveButton);
    actionRow->addWidget(checkButton);
    actionRow->addStretch();

    auto* closeBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    connect(closeBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(warningsLabel);
    layout->addWidget(m_warningsView);
    layout->addWidget(jsonLabel);
    layout->addWidget(m_editor);
    layout->addLayout(actionRow);
    layout->addWidget(closeBox);
}

void SingBoxConfigPreviewDialog::onCopy()
{
    QApplication::clipboard()->setText(m_jsonText);
}

void SingBoxConfigPreviewDialog::onSaveAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save sing-box config"), AppPaths::singBoxTunConfigPath(),
        QStringLiteral("JSON (*.json);;All files (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), file.errorString());
        return;
    }
    file.write(m_jsonText.toUtf8());
}

void SingBoxConfigPreviewDialog::onRunCheck()
{
    if (!m_coreManager) {
        QMessageBox::warning(this, QStringLiteral("sing-box check"),
                             QStringLiteral("Core manager is not available."));
        return;
    }

    const QString configPath = AppPaths::singBoxTunConfigPath();
    QFile configFile(configPath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("sing-box check"), configFile.errorString());
        return;
    }
    configFile.write(m_jsonText.toUtf8());
    configFile.close();

    const CoreValidationResult validation = m_coreManager->validateSingBoxConfig(
        AppSettings::instance().resolvedSingBoxPath(), configPath);
    QString message = validation.success ? QStringLiteral("sing-box check OK.")
                                         : validation.errorMessage;
    if (!validation.output.isEmpty()) {
        message += QStringLiteral("\n\n") + validation.output;
    }
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("sing-box check"));
    box.setIcon(validation.success ? QMessageBox::Information : QMessageBox::Warning);
    box.setText(message);
    box.exec();
}

} // namespace zarya
