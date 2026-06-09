#include "ui/BetaBannerWidget.h"

#include "storage/AppSettings.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

namespace zarya {

BetaBannerWidget::BetaBannerWidget(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet(QStringLiteral("background:#fff3e0; border-bottom:1px solid #ffcc80;"));
    m_label = new QLabel(
        tr("Zarya beta — experimental features may break networking. "
           "Use Diagnostics Bundle when reporting issues."),
        this);
    m_label->setWordWrap(true);
    auto* dismiss = new QPushButton(tr("Dismiss"), this);
    connect(dismiss, &QPushButton::clicked, this, [this]() {
        AppSettings::instance().setDismissBetaBanner(true);
        hide();
        emit dismissed();
    });
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(m_label, 1);
    layout->addWidget(dismiss);
}

} // namespace zarya
