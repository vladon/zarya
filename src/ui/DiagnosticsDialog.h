#pragma once

#include "diagnostics/DiagnosticsManager.h"

#include <QDialog>
#include <functional>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QRadioButton;

namespace zarya {

class DiagnosticsDialog : public QDialog {
    Q_OBJECT

public:
    explicit DiagnosticsDialog(DiagnosticsManager& manager,
                               const std::function<void(const QString&)>& logCallback,
                               QWidget* parent = nullptr);

private slots:
    void onBrowse();
    void onPreview();
    void onCreate();

private:
    DiagnosticsOptions buildOptions() const;

    DiagnosticsManager& m_manager;
    std::function<void(const QString&)> m_logCallback;

    QRadioButton* m_strictRedactionRadio = nullptr;
    QRadioButton* m_basicRedactionRadio = nullptr;
    QCheckBox* m_runValidationCheck = nullptr;
    QCheckBox* m_extendedLogsCheck = nullptr;
    QCheckBox* m_machinePathsCheck = nullptr;
    QLineEdit* m_outputEdit = nullptr;
    QPushButton* m_createButton = nullptr;
};

} // namespace zarya
