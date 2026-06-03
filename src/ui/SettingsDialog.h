#pragma once

#include <QDialog>

class QLineEdit;
class QSpinBox;

namespace zarya {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onBrowseXray();

private:
    QLineEdit* m_xrayPathEdit = nullptr;
    QSpinBox* m_socksPortSpin = nullptr;
    QSpinBox* m_httpPortSpin = nullptr;
};

} // namespace zarya
