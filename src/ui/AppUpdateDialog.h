#pragma once

#include "updater/AppUpdateChecker.h"
#include "updater/AppUpdatePlanner.h"

#include <QDialog>

class QLabel;
class QPlainTextEdit;
class QPushButton;

namespace zarya {

class AppUpdateDialog : public QDialog {
    Q_OBJECT

public:
    explicit AppUpdateDialog(QWidget* parent = nullptr);

private slots:
    void onCheckNow();
    void onChooseLocalManifest();
    void onDownloadAndVerify();
    void onOpenDownloadsFolder();
    void onCheckStarted();
    void onCheckFinished(const AppUpdatePlan& plan);
    void onCheckFailed(const QString& error);

private:
    void refreshStaticInfo();
    void setStatusText(const QString& text);
    void setBusy(bool busy);
    void updatePlanView(const AppUpdatePlan& plan);
    QString installStatusMessage() const;

    AppUpdateChecker m_checker;
    AppUpdatePlan m_lastPlan;
    bool m_hasPlan = false;

    QLabel* m_currentVersionLabel = nullptr;
    QLabel* m_channelLabel = nullptr;
    QLabel* m_installationModeLabel = nullptr;
    QLabel* m_manifestLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPlainTextEdit* m_detailsText = nullptr;
    QPushButton* m_checkButton = nullptr;
    QPushButton* m_chooseManifestButton = nullptr;
    QPushButton* m_downloadButton = nullptr;
    QPushButton* m_openDownloadsButton = nullptr;
};

} // namespace zarya
