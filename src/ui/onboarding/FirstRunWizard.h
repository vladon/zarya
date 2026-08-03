#pragma once

#include "ui/onboarding/FirstRunState.h"

#include <QDialog>

class QStackedLayout;
class QShowEvent;

namespace zarya {

class CoreBinaryManager;
class DnsManager;
class FirstRunChecklistWidget;
class ProfileImportWidget;
class RoutingManager;
class ZaryaActionButton;
class ZaryaBodyText;
class ZaryaCheckBox;
class ZaryaSelector;
class ZaryaTextField;

class FirstRunWizard : public QDialog {
    Q_OBJECT

public:
    explicit FirstRunWizard(CoreBinaryManager* coreManager, RoutingManager* routingManager,
                            DnsManager* dnsManager, QWidget* parent = nullptr);

    FirstRunState state() const;
    bool wasSkipped() const;

Q_SIGNALS:
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
    void showEvent(QShowEvent* event) override;

private:
    void setupPages();
    void refreshCorePage();
    void refreshFinishPage(bool announce);
    bool validatePage();
    void showPage(int index);
    void activateCurrentPageAccessibility();

    CoreBinaryManager* m_coreManager = nullptr;
    RoutingManager* m_routingManager = nullptr;
    DnsManager* m_dnsManager = nullptr;
    ZaryaBodyText* m_coreStatus = nullptr;
    QStackedLayout* m_pages = nullptr;
    ZaryaActionButton* m_back = nullptr;
    ZaryaActionButton* m_next = nullptr;
    ZaryaActionButton* m_installXray = nullptr;
    ProfileImportWidget* m_importWidget = nullptr;
    ZaryaTextField* m_subscriptionUrl = nullptr;
    ZaryaTextField* m_subscriptionName = nullptr;
    ZaryaSelector* m_routing = nullptr;
    ZaryaSelector* m_dns = nullptr;
    ZaryaSelector* m_runtime = nullptr;
    ZaryaCheckBox* m_tunAccepted = nullptr;
    FirstRunChecklistWidget* m_checklist = nullptr;
    ZaryaCheckBox* m_startNow = nullptr;
    QVector<QWidget*> m_pageFocusTargets;
    FirstRunState m_state;
    bool m_skipped = false;
};

} // namespace zarya
