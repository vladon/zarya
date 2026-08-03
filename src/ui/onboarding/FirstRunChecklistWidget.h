#pragma once

#include <QWidget>

namespace zarya {

struct FirstRunState;
class ZaryaBodyText;

class FirstRunChecklistWidget : public QWidget {
    Q_OBJECT

public:
    explicit FirstRunChecklistWidget(QWidget* parent = nullptr);

    void updateFromState(const FirstRunState& state, int profileCount, bool xrayInstalled,
                         const QString& xrayVersion, bool announce = true);

private:
    ZaryaBodyText* m_body = nullptr;
};

} // namespace zarya
