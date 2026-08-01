#pragma once

#include <QByteArray>
#include <QList>
#include <QWidget>

class QResizeEvent;

namespace zarya {

class ZaryaSplitView final : public QWidget {
    Q_OBJECT

public:
    ZaryaSplitView(QWidget* first, QWidget* second, QWidget* parent = nullptr);

    void setSizes(const QList<int>& sizes);
    [[nodiscard]] QList<int> sizes() const;
    [[nodiscard]] QByteArray saveState() const;
    bool restoreState(const QByteArray& state);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setFirstExtent(int extent);
    void relayout();
    [[nodiscard]] int availableExtent() const;

    QWidget* m_first = nullptr;
    QWidget* m_second = nullptr;
    QWidget* m_handle = nullptr; // Ui::RpWidget*
    double m_ratio = 0.75;
};

} // namespace zarya
