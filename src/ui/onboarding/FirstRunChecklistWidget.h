#pragma once

#include <QWidget>

class QLabel;

namespace zarya {

struct FirstRunState;

class FirstRunChecklistWidget : public QWidget {
    Q_OBJECT

public:
    explicit FirstRunChecklistWidget(QWidget* parent = nullptr);

    void updateFromState(const FirstRunState& state, int profileCount, bool xrayInstalled,
                         const QString& xrayVersion);

private:
    QLabel* m_body = nullptr;
};

} // namespace zarya
