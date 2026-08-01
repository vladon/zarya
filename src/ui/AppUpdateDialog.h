#pragma once

#include "updater/AppUpdateChecker.h"
#include "updater/AppUpdatePlanner.h"

#include <QDialog>
#include <functional>

namespace zarya {

class AppController;
class ZaryaActionButton;
class ZaryaBodyText;
class ZaryaTextArea;

class AppUpdateDialog : public QDialog {
    Q_OBJECT

public:
    explicit AppUpdateDialog(AppController* controller,
                             const std::function<bool()>& isTestsRunning,
                             QWidget* parent = nullptr);

private Q_SLOTS:
    void onCheckNow();
    void onChooseLocalManifest();
    void onDownloadAndVerify();
    void onInstallAndRestart();
    void onOpenDownloadsFolder();
    void onCheckStarted();
    void onCheckFinished(const AppUpdatePlan& plan);
    void onCheckFailed(const QString& error);

private:
    void refreshStaticInfo();
    void setStatusText(const QString& text);
    void setBusy(bool busy);
    void updatePlanView(const AppUpdatePlan& plan);
    void refreshInstallButtonState();
    QString installStatusMessage() const;
    bool killSwitchActive() const;

    AppController* m_controller = nullptr;
    std::function<bool()> m_isTestsRunning;
    AppUpdateChecker m_checker;
    AppUpdatePlan m_lastPlan;
    bool m_hasPlan = false;
    bool m_artifactVerified = false;
    bool m_stagingReady = false;
    QString m_verifiedArchivePath;
    QString m_stagingDir;

    ZaryaBodyText* m_currentVersionLabel = nullptr;
    ZaryaBodyText* m_channelLabel = nullptr;
    ZaryaBodyText* m_installationModeLabel = nullptr;
    ZaryaBodyText* m_manifestLabel = nullptr;
    ZaryaBodyText* m_statusLabel = nullptr;
    ZaryaTextArea* m_detailsText = nullptr;
    ZaryaActionButton* m_checkButton = nullptr;
    ZaryaActionButton* m_chooseManifestButton = nullptr;
    ZaryaActionButton* m_downloadButton = nullptr;
    ZaryaActionButton* m_installButton = nullptr;
    ZaryaActionButton* m_openDownloadsButton = nullptr;
};

} // namespace zarya
