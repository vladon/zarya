#include "ui/BetaBannerWidget.h"

#include "packaging/PackagingInfo.h"
#include "storage/AppSettings.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

namespace zarya {

BetaBannerWidget::BetaBannerWidget(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet(QStringLiteral("background:#fff3e0; border-bottom:1px solid #ffcc80;"));
    QString bannerText;
    if (PackagingInfo::isReleaseCandidateBuild()) {
        if (AppSettings::instance().showExperimentalFeatures()) {
            bannerText =
                tr("Experimental features are enabled. They are not part of the stable support "
                   "scope. Use Diagnostics Bundle when reporting issues.");
        } else {
            bannerText =
                tr("Zarya release candidate — experimental features are disabled by default. "
                   "Use Diagnostics Bundle when reporting issues.");
        }
    } else {
        bannerText = tr("Zarya beta — experimental features may break networking. "
                        "Use Diagnostics Bundle when reporting issues.");
    }
    m_label = new QLabel(bannerText, this);
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
