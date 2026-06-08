#pragma once

#include "diagnostics/DiagnosticsManager.h"
#include "diagnostics/DiagnosticsOptions.h"

#include <QDialog>

class QLabel;
class QListWidget;
class QPushButton;

namespace zarya {

class DiagnosticsPreviewDialog : public QDialog {
    Q_OBJECT

public:
    DiagnosticsPreviewDialog(const DiagnosticsPreviewResult& preview, QWidget* parent = nullptr);

private:
    QLabel* m_redactionLabel = nullptr;
    QLabel* m_warningsLabel = nullptr;
    QListWidget* m_filesList = nullptr;
};

} // namespace zarya
