#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

class QAction;

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
        const QString& moreText,
        QWidget* parent = nullptr);
    ~ProfileActionStrip() override;

    [[nodiscard]] ZaryaSelector* profileSelector() const;

private:
    void bindButton(QWidget* button, QAction* action);
    void rebuildOverflowMenu();

    ProfileActionStripActions m_actions;
    ZaryaSelector* m_profileSelector = nullptr;
    QWidget* m_startButton = nullptr; // Ui::RoundButton*
    QWidget* m_stopButton = nullptr; // Ui::RoundButton*
    QWidget* m_moreButton = nullptr; // Ui::RoundButton*
    QWidget* m_menu = nullptr; // Ui::DropdownMenu*
};

} // namespace zarya
