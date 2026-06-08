#pragma once

#include <QDialog>

class QCheckBox;

namespace zarya {

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
    QCheckBox* m_stopRuntimeCheck = nullptr;
    QCheckBox* m_restoreProxyCheck = nullptr;
    QCheckBox* m_disableKillSwitchCheck = nullptr;
};

} // namespace zarya
