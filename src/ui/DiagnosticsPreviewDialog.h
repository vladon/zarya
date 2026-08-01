#pragma once

#include "diagnostics/DiagnosticsManager.h"
#include "diagnostics/DiagnosticsOptions.h"

#include <QDialog>

class QListWidget;

namespace zarya {

class ZaryaBodyText;

class DiagnosticsPreviewDialog : public QDialog {
    Q_OBJECT

public:
    DiagnosticsPreviewDialog(const DiagnosticsPreviewResult& preview, QWidget* parent = nullptr);

private:
    ZaryaBodyText* m_redactionLabel = nullptr;
    ZaryaBodyText* m_warningsLabel = nullptr;
    QListWidget* m_filesList = nullptr;
};

} // namespace zarya
