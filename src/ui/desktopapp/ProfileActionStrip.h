#pragma once

#include <QString>
#include <QPointer>
#include <QVector>
#include <QWidget>

class QAction;
class QHBoxLayout;
class QResizeEvent;

namespace zarya {

class ZaryaSelector;

struct ProfileActionStripActions {
    QAction* add = nullptr;
    QAction* importProfiles = nullptr;
    QAction* subscriptions = nullptr;
    QAction* testSelected = nullptr;
    QAction* start = nullptr;
    QAction* stop = nullptr;
    QVector<QAction*> overflow;
};

class ProfileActionStrip final : public QWidget {
    Q_OBJECT

public:
    ProfileActionStrip(
        ProfileActionStripActions actions,
        const QString& profileSelectorLabel,
        const QString& moreText,
        QWidget* parent = nullptr);
    ~ProfileActionStrip() override;

    [[nodiscard]] ZaryaSelector* profileSelector() const;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void bindButton(QWidget* button, QAction* action);
    void rebuildOverflowMenu();
    void updateResponsiveVisibility();

    ProfileActionStripActions m_actions;
    QHBoxLayout* m_layout = nullptr;
    ZaryaSelector* m_profileSelector = nullptr;
    QWidget* m_importButton = nullptr; // Ui::RoundButton*
    QWidget* m_subscriptionsButton = nullptr; // Ui::RoundButton*
    QWidget* m_testButton = nullptr; // Ui::RoundButton*
    QWidget* m_moreButton = nullptr; // Ui::RoundButton*
    QPointer<QWidget> m_menu; // Ui::DropdownMenu*
};

} // namespace zarya
