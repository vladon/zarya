#pragma once

#include "ui/desktopapp/ZaryaAccessibleFormControl.h"

#include <QString>
#include <QPointer>
#include <QVector>
#include <QWidget>

class QResizeEvent;

namespace zarya {

struct ZaryaSelectorItem {
    QString key;
    QString text;
    bool enabled = true;
};

class ZaryaSelector final : public QWidget, public ZaryaAccessibleFormControl {
    Q_OBJECT

public:
    explicit ZaryaSelector(QWidget* parent = nullptr);
    ~ZaryaSelector() override;

    void setItems(QVector<ZaryaSelectorItem> items, const QString& currentKey = {});
    bool setCurrentKey(const QString& key);
    [[nodiscard]] QString currentKey() const;
    void setAccessibleLabel(const QString& label) override;

Q_SIGNALS:
    void currentKeyChanged(const QString& key);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyCurrentItem();
    void toggleMenu();

    QVector<ZaryaSelectorItem> m_items;
    QString m_currentKey;
    QString m_accessibleLabel;
    QWidget* m_button = nullptr; // Ui::RoundButton*
    QPointer<QWidget> m_menu; // Ui::DropdownMenu*
};

} // namespace zarya
