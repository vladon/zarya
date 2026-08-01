#pragma once

#include "domain/DnsServer.h"

#include <QDialog>

namespace zarya {

class ZaryaCheckBox;
class ZaryaNumberField;
class ZaryaSelector;
class ZaryaTextArea;
class ZaryaTextField;

class DnsServerEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit DnsServerEditorDialog(const DnsServer& server, QWidget* parent = nullptr);

    DnsServer server() const;

private:
    DnsServer m_server;
    ZaryaTextField* m_addressEdit = nullptr;
    ZaryaSelector* m_kindCombo = nullptr;
    ZaryaNumberField* m_portSpin = nullptr;
    ZaryaTextArea* m_domainsEdit = nullptr;
    ZaryaTextArea* m_expectIpsEdit = nullptr;
    ZaryaTextField* m_tagEdit = nullptr;
    ZaryaNumberField* m_timeoutSpin = nullptr;
    ZaryaCheckBox* m_skipFallbackCheck = nullptr;
    ZaryaTextField* m_noteEdit = nullptr;
};

} // namespace zarya
