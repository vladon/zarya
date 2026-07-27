#pragma once

#include <QString>
#include <QWidget>

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

class StatusBadgeLibUiEmbed;

class StatusBadge : public QWidget {
    Q_OBJECT

public:
    explicit StatusBadge(QWidget* parent = nullptr);

    void setKind(StatusBadgeKind kind);
    void setBadgeText(const QString& text);

private:
    StatusBadgeKind m_kind = StatusBadgeKind::Neutral;
    StatusBadgeLibUiEmbed* m_embed = nullptr;
};

} // namespace zarya
