#pragma once

#include <QDialog>

namespace zarya {

class LibUiSpikeDialog : public QDialog {
    Q_OBJECT

public:
    explicit LibUiSpikeDialog(QWidget* parent = nullptr);
};

} // namespace zarya
