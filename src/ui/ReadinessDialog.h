#pragma once

#include <QDialog>

namespace zarya {

class ReadinessDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReadinessDialog(QWidget* parent = nullptr);

signals:
    void openCoreManagerRequested();
    void importProfileRequested();
    void openSettingsRequested();
};

} // namespace zarya
