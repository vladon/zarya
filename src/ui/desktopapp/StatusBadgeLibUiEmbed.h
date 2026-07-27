#pragma once

#include "ui/widgets/StatusBadge.h"

#include <QWidget>

namespace zarya {

// lib_ui FlatLabel badge host. Header stays free of toolkit includes so
// StatusBadge.cpp can use it without QT_NO_KEYWORDS.
class StatusBadgeLibUiEmbed : public QWidget {
    Q_OBJECT

public:
    explicit StatusBadgeLibUiEmbed(QWidget* parent = nullptr);

    void setKind(StatusBadgeKind kind);
    void setBadgeText(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyColors();
    void layoutLabel();

    StatusBadgeKind m_kind = StatusBadgeKind::Neutral;
    QColor m_bg;
    QColor m_fg;
    QWidget* m_labelHost = nullptr; // Ui::FlatLabel*, opaque here
};

} // namespace zarya
