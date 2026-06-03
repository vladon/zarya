#pragma once

#include "domain/DnsProfile.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;
class QTabWidget;

namespace zarya {

class DnsProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit DnsProfileDialog(const DnsProfile& profile, bool readOnly, QWidget* parent = nullptr);

    DnsProfile profile() const;

private slots:
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

    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_modeCombo = nullptr;
    QCheckBox* m_enabledCheck = nullptr;
    QComboBox* m_queryStrategyCombo = nullptr;
    QTableWidget* m_serversTable = nullptr;
    QPlainTextEdit* m_hostsEdit = nullptr;
    QCheckBox* m_disableCacheCheck = nullptr;
    QCheckBox* m_disableFallbackCheck = nullptr;
    QCheckBox* m_disableFallbackIfMatchCheck = nullptr;
};

} // namespace zarya
