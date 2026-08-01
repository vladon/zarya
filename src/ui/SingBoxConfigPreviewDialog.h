#pragma once

#include <QDialog>

namespace zarya {

class CoreManager;
class ZaryaTextArea;

class SingBoxConfigPreviewDialog : public QDialog {
    Q_OBJECT

public:
    SingBoxConfigPreviewDialog(const QString& jsonText, const QStringList& warnings,
                               CoreManager* coreManager, QWidget* parent = nullptr);

private Q_SLOTS:
    void onCopy();
    void onSaveAs();
    void onRunCheck();

private:
    ZaryaTextArea* m_editor = nullptr;
    ZaryaTextArea* m_warningsView = nullptr;
    QString m_jsonText;
    CoreManager* m_coreManager = nullptr;
};

} // namespace zarya
