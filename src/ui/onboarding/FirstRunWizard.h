#pragma once

#include "ui/onboarding/FirstRunState.h"

#include <QWizard>

namespace zarya {

class CoreBinaryManager;
class RoutingManager;
class DnsManager;

class FirstRunWizard : public QWizard {
    Q_OBJECT

public:
    explicit FirstRunWizard(CoreBinaryManager* coreManager, RoutingManager* routingManager,
                            DnsManager* dnsManager, QWidget* parent = nullptr);

    FirstRunState state() const;
    bool wasSkipped() const;

signals:
    void openCoreManagerRequested();
    void chooseXrayBinaryRequested();
    void chooseSingBoxBinaryRequested();
    void installXrayRequested();
    void installSingBoxRequested();
    void openRoutingProfilesRequested();
    void openDnsProfilesRequested();
    void importBackupRequested();
    void addProfileManuallyRequested();
    void configureHelperRequested();
    void wizardFinishedState(const FirstRunState& state);

protected:
    void accept() override;
    void reject() override;

private:
    void setupPages();
    void refreshCorePage();
    bool validateCurrentPage() override;

    CoreBinaryManager* m_coreManager = nullptr;
    RoutingManager* m_routingManager = nullptr;
    DnsManager* m_dnsManager = nullptr;
    FirstRunState m_state;
    bool m_skipped = false;
};

} // namespace zarya
