#include "ui/RuleSetManagerDialog.h"

#include "dns/DnsManager.h"
#include "routing/RoutingManager.h"
#include "rulesets/RuleSetStatus.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "ui/RuleSetManagerAccessibility.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPlainTextEdit>
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

    auto* targetLabel = new ZaryaBodyText(
        tr("Target directory: %1").arg(m_manager.targetDirectory()), this);

    auto* singBoxLabel = new ZaryaBodyText(
        tr("sing-box executable: %1")
            .arg(AppSettings::instance().resolvedSingBoxPath()),
        this);

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

    auto* checkButton = new ZaryaActionButton(tr("Check Status"), this);
    m_importButton = new ZaryaActionButton(tr("Import Local .srs"), this);
    m_compileButton = new ZaryaActionButton(tr("Compile JSON"), this);
    auto* openFolderButton = new ZaryaActionButton(tr("Open Folder"), this);
    auto* closeButton = new ZaryaActionButton(tr("Close"), this);

    connect(checkButton, &ZaryaActionButton::clicked,
            this, &RuleSetManagerDialog::onCheckStatus);
    connect(m_importButton, &ZaryaActionButton::clicked,
            this, &RuleSetManagerDialog::onImportSrs);
    connect(m_compileButton, &ZaryaActionButton::clicked,
            this, &RuleSetManagerDialog::onCompileJson);
    connect(openFolderButton, &ZaryaActionButton::clicked,
            this, &RuleSetManagerDialog::onOpenFolder);
    connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::accept);
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

    m_requiredEmptyState = new ZaryaBodyText(
        tr("No rule sets are required by the active TUN configuration."), this);
    auto* requiredSection = new ZaryaFormSection(
        tr("Required by active TUN config"), this);
    requiredSection->addWidget(m_requiredEmptyState);
    requiredSection->addWidget(m_requiredTable);

    m_allEmptyState = new ZaryaBodyText(tr("No rule sets are available."), this);
    auto* allSection = new ZaryaFormSection(tr("All rule sets"), this);
    allSection->addWidget(m_allEmptyState);
    allSection->addWidget(m_allTable);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(targetLabel);
    layout->addWidget(singBoxLabel);
    layout->addWidget(requiredSection);
    layout->addWidget(allSection);
    layout->addWidget(m_logView);
    layout->addLayout(buttonRow);

    onCheckStatus();
    configureRuleSetManagerAccessibility(
        m_requiredTable,
        m_allTable,
        m_logView,
        {checkButton, m_importButton, m_compileButton, openFolderButton, closeButton},
        tr("Required rule sets"),
        tr("All rule sets"),
        tr("Rule set log"));
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
    m_requiredEmptyState->setVisible(required.isEmpty());
    m_requiredTable->setVisible(!required.isEmpty());
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
    m_allEmptyState->setVisible(items.isEmpty());
    m_allTable->setVisible(!items.isEmpty());
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
        UiMessagePresenter::information(
            this, tr("Import"), tr("Select a rule set row first."));
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
        UiMessagePresenter::warning(this, tr("Import"), error);
        return;
    }
    refreshTables();
}

void RuleSetManagerDialog::onCompileJson()
{
    const int row = m_allTable->currentRow();
    if (row < 0) {
        UiMessagePresenter::information(
            this, tr("Compile"), tr("Select a rule set row first."));
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
        UiMessagePresenter::warning(this, tr("Compile"), error);
        return;
    }
    refreshTables();
}

void RuleSetManagerDialog::onOpenFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_manager.targetDirectory()));
}

} // namespace zarya
