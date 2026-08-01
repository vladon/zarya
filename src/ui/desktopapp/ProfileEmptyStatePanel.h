#pragma once

#include <QString>
#include <QWidget>

class QResizeEvent;

namespace zarya {

class ProfileEmptyStatePanel final : public QWidget {
    Q_OBJECT

public:
    explicit ProfileEmptyStatePanel(QWidget* parent = nullptr);

    void setMessage(const QString& message);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void relayout();

    QWidget* m_label = nullptr; // Ui::FlatLabel*
};

} // namespace zarya
