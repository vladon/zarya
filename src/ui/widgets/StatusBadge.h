#pragma once

#include <QString>
#include <QWidget>

class QLabel;

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

#if defined(ZARYA_DESKTOP_APP_UI)
class StatusBadgeLibUiEmbed;
#endif

class StatusBadge : public QWidget {
    Q_OBJECT

public:
    explicit StatusBadge(QWidget* parent = nullptr);

    void setKind(StatusBadgeKind kind);
    void setBadgeText(const QString& text);

private:
    void applyStyle(StatusBadgeKind kind);

    StatusBadgeKind m_kind = StatusBadgeKind::Neutral;
#if defined(ZARYA_DESKTOP_APP_UI)
    StatusBadgeLibUiEmbed* m_embed = nullptr;
#else
    QLabel* m_label = nullptr;
#endif
};

} // namespace zarya
