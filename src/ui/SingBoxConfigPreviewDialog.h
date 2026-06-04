#pragma once

#include "runtime/ConfigWarning.h"

#include <QDialog>

class QPlainTextEdit;

namespace zarya {

class CoreManager;

class SingBoxConfigPreviewDialog : public QDialog {
    Q_OBJECT

public:
    SingBoxConfigPreviewDialog(const QString& jsonText, const QStringList& warnings,
                               CoreManager* coreManager, QWidget* parent = nullptr);

private slots:
    void onCopy();
    void onSaveAs();
    void onRunCheck();

private:
    QPlainTextEdit* m_editor = nullptr;
    QPlainTextEdit* m_warningsView = nullptr;
    QString m_jsonText;
    CoreManager* m_coreManager = nullptr;
};

} // namespace zarya
