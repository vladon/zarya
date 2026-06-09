#include "ui/RuleSetManagerDialog.h"

#include "dns/DnsManager.h"
#include "routing/RoutingManager.h"
#include "rulesets/RuleSetStatus.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace zarya {

RuleSetManagerDialog::RuleSetManagerDialog(RuleSetManager& manager, RoutingManager& routingManager,
                                           DnsManager& dnsManager,
                                           const std::function<void(const QString&)>& logCallback,
                                           QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_routingManager(routingManager)
    , m_dnsManager(dnsManager)
    , m_logCallback(logCallback)
{
    setWindowTitle(tr("sing-box Rule Sets"));
    resize(960, 620);

    auto* targetLabel = new QLabel(
        tr("Target directory: %1").arg(m_manager.targetDirectory()), this);
    targetLabel->setWordWrap(true);
    targetLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto* singBoxLabel = new QLabel(
        tr("sing-box executable: %1")
            .arg(AppSettings::instance().resolvedSingBoxPath()),
        this);
    singBoxLabel->setWordWrap(true);

    m_requiredTable = new QTableWidget(this);
    m_requiredTable->setColumnCount(4);
    m_requiredTable->setHorizontalHeaderLabels(
        {tr("Tag"), tr("Source"), tr("Status"),
         tr("Path")});
    m_requiredTable->horizontalHeader()->setStretchLastSection(true);
    m_requiredTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_requiredTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_allTable = new QTableWidget(this);
    m_allTable->setColumnCount(6);
    m_allTable->setHorizontalHeaderLabels(
        {tr("Tag"), tr("Kind"), tr("Status"),
         tr("Source"), tr("Size"), tr("Modified")});
    m_allTable->horizontalHeader()->setStretchLastSection(true);
    m_allTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_allTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(300);

    auto* checkButton = new QPushButton(tr("Check Status"), this);
    m_importButton = new QPushButton(tr("Import Local .srs"), this);
    m_compileButton = new QPushButton(tr("Compile JSON"), this);
    auto* openFolderButton = new QPushButton(tr("Open Folder"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);

    connect(checkButton, &QPushButton::clicked, this, &RuleSetManagerDialog::onCheckStatus);
    connect(m_importButton, &QPushButton::clicked, this, &RuleSetManagerDialog::onImportSrs);
    connect(m_compileButton, &QPushButton::clicked, this, &RuleSetManagerDialog::onCompileJson);
    connect(openFolderButton, &QPushButton::clicked, this, &RuleSetManagerDialog::onOpenFolder);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(&m_manager, &RuleSetManager::logLine, this, [this](const QString& line) {
        m_logView->appendPlainText(line);
        if (m_logCallback) {
            m_logCallback(line);
        }
    });
    connect(&m_manager, &RuleSetManager::itemsChanged, this, &RuleSetManagerDialog::refreshTables);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(checkButton);
    buttonRow->addWidget(m_importButton);
    buttonRow->addWidget(m_compileButton);
    buttonRow->addWidget(openFolderButton);
    buttonRow->addStretch();
    buttonRow->addWidget(closeButton);

    auto* requiredGroup = new QGroupBox(tr("Required by active TUN config"), this);
    auto* requiredLayout = new QVBoxLayout(requiredGroup);
    requiredLayout->addWidget(m_requiredTable);

    auto* allGroup = new QGroupBox(tr("All rule sets"), this);
    auto* allLayout = new QVBoxLayout(allGroup);
    allLayout->addWidget(m_allTable);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(targetLabel);
    layout->addWidget(singBoxLabel);
    layout->addWidget(requiredGroup);
    layout->addWidget(allGroup);
    layout->addWidget(m_logView);
    layout->addLayout(buttonRow);

    onCheckStatus();
}

void RuleSetManagerDialog::onCheckStatus()
{
    m_manager.reload();
    refreshTables();
}

void RuleSetManagerDialog::onRefreshRequired()
{
    refreshTables();
}

void RuleSetManagerDialog::refreshTables()
{
    const RoutingProfile routing = m_routingManager.activeProfile();
    const DnsProfile dns = m_dnsManager.activeProfile();
    const QVector<RequiredRuleSet> required = m_manager.detectRequired(routing, dns);

    m_requiredTable->setRowCount(required.size());
    for (int row = 0; row < required.size(); ++row) {
        const RequiredRuleSet& entry = required.at(row);
        m_requiredTable->setItem(row, 0, new QTableWidgetItem(entry.tag));
        m_requiredTable->setItem(row, 1, new QTableWidgetItem(entry.sourceArea));
        m_requiredTable->setItem(
            row, 2,
            new QTableWidgetItem(entry.available ? tr("present")
                                                 : ruleSetStatusDisplayName(entry.catalogStatus)));
        m_requiredTable->setItem(row, 3, new QTableWidgetItem(entry.localPath));
    }

    const QVector<RuleSetItem> items = m_manager.items();
    m_allTable->setRowCount(items.size());
    for (int row = 0; row < items.size(); ++row) {
        const RuleSetItem& item = items.at(row);
        m_allTable->setItem(row, 0, new QTableWidgetItem(item.tag));
        m_allTable->setItem(row, 1, new QTableWidgetItem(ruleSetKindToString(item.kind)));
        m_allTable->setItem(row, 2, new QTableWidgetItem(ruleSetStatusDisplayName(item.status)));
        m_allTable->setItem(row, 3,
                            new QTableWidgetItem(item.builtIn ? tr("Built-in")
                                                              : tr("Custom")));
        m_allTable->setItem(row, 4,
                            new QTableWidgetItem(item.sizeBytes > 0 ? formatBytes(item.sizeBytes)
                                                                    : QStringLiteral("-")));
        m_allTable->setItem(
            row, 5,
            new QTableWidgetItem(item.modifiedAt.isValid()
                                     ? item.modifiedAt.toString(Qt::ISODate)
                                     : QStringLiteral("-")));
    }
}

QString RuleSetManagerDialog::formatBytes(qint64 bytes) const
{
    if (bytes < 1024) {
        return tr("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return tr("%1 KB").arg(bytes / 1024);
    }
    return tr("%1 MB").arg(bytes / (1024 * 1024));
}

void RuleSetManagerDialog::onImportSrs()
{
    const int row = m_allTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Import"),
                                 tr("Select a rule set row first."));
        return;
    }
    const QString tag = m_allTable->item(row, 0)->text();
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Import .srs"), {}, tr("sing-box rule set (*.srs)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!m_manager.importLocalSrs(tag, path, &error)) {
        QMessageBox::warning(this, tr("Import"), error);
        return;
    }
    refreshTables();
}

void RuleSetManagerDialog::onCompileJson()
{
    const int row = m_allTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Compile"),
                                 tr("Select a rule set row first."));
        return;
    }
    const QString tag = m_allTable->item(row, 0)->text();
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Select source JSON"), AppPaths::singBoxRuleSetSourceDir(),
        tr("JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!m_manager.importLocalJson(tag, path, &error)) {
        QMessageBox::warning(this, tr("Compile"), error);
        return;
    }
    refreshTables();
}

void RuleSetManagerDialog::onOpenFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_manager.targetDirectory()));
}

} // namespace zarya
