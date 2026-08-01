#pragma once

#include <QDialog>

namespace zarya {

class ZaryaCheckBox;

struct SafeExitOptions {
    bool stopRuntime = true;
    bool restoreSystemProxy = true;
    bool disableKillSwitch = true;
};

class SafeExitDialog : public QDialog {
    Q_OBJECT

public:
    explicit SafeExitDialog(QWidget* parent = nullptr);

    SafeExitOptions options() const;

private:
    ZaryaCheckBox* m_stopRuntimeCheck = nullptr;
    ZaryaCheckBox* m_restoreProxyCheck = nullptr;
    ZaryaCheckBox* m_disableKillSwitchCheck = nullptr;
};

} // namespace zarya
