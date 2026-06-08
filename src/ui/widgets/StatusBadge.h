#pragma once

#include <QLabel>

namespace zarya {

enum class StatusBadgeKind {
    Ok,
    Warning,
    Error,
    Experimental,
    Unsupported,
    Running,
    Stopped,
    Neutral,
};

class StatusBadge : public QLabel {
    Q_OBJECT

public:
    explicit StatusBadge(QWidget* parent = nullptr);

    void setKind(StatusBadgeKind kind);
    void setBadgeText(const QString& text);

private:
    void applyStyle(StatusBadgeKind kind);
};

} // namespace zarya
