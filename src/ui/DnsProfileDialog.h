#pragma once

#include "domain/DnsProfile.h"

#include <QDialog>

class QTableWidget;
class QStackedLayout;

namespace zarya {

class ZaryaBodyText;
class ZaryaCheckBox;
class ZaryaSelector;
class ZaryaTextArea;
class ZaryaTextField;

class DnsProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit DnsProfileDialog(const DnsProfile& profile, bool readOnly, QWidget* parent = nullptr);

    DnsProfile profile() const;

private Q_SLOTS:
    void onAddServer();
    void onEditServer();
    void onDeleteServer();
    void onMoveUp();
    void onMoveDown();
    void onValidate();
    void onPreviewDnsJson();
    void refreshServersTable();

private:
    QMap<QString, QString> parseHostsText(const QString& text) const;
    QString hostsToText(const QMap<QString, QString>& hosts) const;

    DnsProfile m_profile;
    bool m_readOnly = false;

    ZaryaTextField* m_nameEdit = nullptr;
    ZaryaSelector* m_modeCombo = nullptr;
    ZaryaCheckBox* m_enabledCheck = nullptr;
    ZaryaSelector* m_queryStrategyCombo = nullptr;
    QTableWidget* m_serversTable = nullptr;
    ZaryaBodyText* m_emptyServers = nullptr;
    ZaryaTextArea* m_hostsEdit = nullptr;
    ZaryaCheckBox* m_disableCacheCheck = nullptr;
    ZaryaCheckBox* m_disableFallbackCheck = nullptr;
    ZaryaCheckBox* m_disableFallbackIfMatchCheck = nullptr;
    QStackedLayout* m_pageStack = nullptr;
};

} // namespace zarya
