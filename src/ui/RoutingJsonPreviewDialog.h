#pragma once

#include <QDialog>

class QPlainTextEdit;

namespace zarya {

class RoutingJsonPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoutingJsonPreviewDialog(const QString& jsonText, QWidget* parent = nullptr);

private:
    QPlainTextEdit* m_editor = nullptr;
};

} // namespace zarya
