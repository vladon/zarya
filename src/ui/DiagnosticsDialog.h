#pragma once

#include "diagnostics/DiagnosticsManager.h"

#include <QDialog>
#include <functional>

namespace zarya {

class ZaryaActionButton;
class ZaryaCheckBox;
class ZaryaRadioGroup;
class ZaryaTextField;

class DiagnosticsDialog : public QDialog {
    Q_OBJECT

public:
    explicit DiagnosticsDialog(DiagnosticsManager& manager,
                               const std::function<void(const QString&)>& logCallback,
                               QWidget* parent = nullptr);

private Q_SLOTS:
    void onBrowse();
    void onPreview();
    void onCreate();

private:
    DiagnosticsOptions buildOptions() const;

    DiagnosticsManager& m_manager;
    std::function<void(const QString&)> m_logCallback;

    ZaryaRadioGroup* m_redactionGroup = nullptr;
    ZaryaCheckBox* m_runValidationCheck = nullptr;
    ZaryaCheckBox* m_extendedLogsCheck = nullptr;
    ZaryaCheckBox* m_machinePathsCheck = nullptr;
    ZaryaTextField* m_outputEdit = nullptr;
    ZaryaActionButton* m_createButton = nullptr;
};

} // namespace zarya
