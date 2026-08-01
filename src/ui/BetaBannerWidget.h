#pragma once

#include <QWidget>

class QPaintEvent;

namespace zarya {

class BetaBannerWidget : public QWidget {
    Q_OBJECT

public:
    explicit BetaBannerWidget(QWidget* parent = nullptr);

Q_SIGNALS:
    void dismissed();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void applyTheme();
};

} // namespace zarya
