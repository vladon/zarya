#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

namespace zarya {

class BetaBannerWidget : public QWidget {
    Q_OBJECT

public:
    explicit BetaBannerWidget(QWidget* parent = nullptr);

signals:
    void dismissed();

private:
    QLabel* m_label = nullptr;
};

} // namespace zarya
