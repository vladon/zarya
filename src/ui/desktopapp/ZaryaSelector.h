#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

class QResizeEvent;

namespace zarya {

struct ZaryaSelectorItem {
    QString key;
    QString text;
    bool enabled = true;
};

class ZaryaSelector final : public QWidget {
    Q_OBJECT

public:
    explicit ZaryaSelector(QWidget* parent = nullptr);
    ~ZaryaSelector() override;

    void setItems(QVector<ZaryaSelectorItem> items, const QString& currentKey = {});
    bool setCurrentKey(const QString& key);
    [[nodiscard]] QString currentKey() const;

Q_SIGNALS:
    void currentKeyChanged(const QString& key);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyCurrentItem();
    void toggleMenu();

    QVector<ZaryaSelectorItem> m_items;
    QString m_currentKey;
    QWidget* m_button = nullptr; // Ui::RoundButton*
    QWidget* m_menu = nullptr; // Ui::DropdownMenu*
};

} // namespace zarya
