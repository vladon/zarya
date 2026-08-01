#pragma once

#include <QDialog>

namespace zarya {

class ZaryaTextArea;

class RoutingJsonPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoutingJsonPreviewDialog(const QString& jsonText, QWidget* parent = nullptr);

private:
    ZaryaTextArea* m_editor = nullptr;
};

} // namespace zarya
