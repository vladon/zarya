#pragma once

#include "domain/DnsServer.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

namespace zarya {

class DnsServerEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit DnsServerEditorDialog(const DnsServer& server, QWidget* parent = nullptr);

    DnsServer server() const;

private:
    DnsServer m_server;
    QLineEdit* m_addressEdit = nullptr;
    QComboBox* m_kindCombo = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QPlainTextEdit* m_domainsEdit = nullptr;
    QPlainTextEdit* m_expectIpsEdit = nullptr;
    QLineEdit* m_tagEdit = nullptr;
    QSpinBox* m_timeoutSpin = nullptr;
    QCheckBox* m_skipFallbackCheck = nullptr;
    QLineEdit* m_noteEdit = nullptr;
};

} // namespace zarya
