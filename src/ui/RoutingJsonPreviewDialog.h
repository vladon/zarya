#pragma once

#include <QDialog>
#include <QShowEvent>

namespace zarya {

class ZaryaTextArea;

class RoutingJsonPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoutingJsonPreviewDialog(const QString& jsonText, QWidget* parent = nullptr);

private:
    void showEvent(QShowEvent* event) override;

    ZaryaTextArea* m_editor = nullptr;
};

} // namespace zarya
