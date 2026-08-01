#pragma once

#include "diagnostics/DiagnosticsManager.h"
#include "diagnostics/DiagnosticsOptions.h"

#include <QDialog>

namespace zarya {

class ZaryaBodyText;
class ZaryaTextArea;

class DiagnosticsPreviewDialog : public QDialog {
    Q_OBJECT

public:
    DiagnosticsPreviewDialog(const DiagnosticsPreviewResult& preview, QWidget* parent = nullptr);

private:
    ZaryaBodyText* m_redactionLabel = nullptr;
    ZaryaBodyText* m_warningsLabel = nullptr;
    ZaryaTextArea* m_filesList = nullptr;
};

} // namespace zarya
