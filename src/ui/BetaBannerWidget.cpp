#include "ui/BetaBannerWidget.h"

#include "packaging/PackagingInfo.h"
#include "storage/AppSettings.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/ThemeTokens.h"

#include <QHBoxLayout>
#include <QPainter>

namespace zarya {

BetaBannerWidget::BetaBannerWidget(QWidget* parent)
    : QWidget(parent)
{
    QString bannerText;
    if (PackagingInfo::isStableBuild()) {
        if (AppSettings::instance().showExperimentalFeatures()) {
            bannerText =
                tr("Experimental features are enabled. They are not part of the stable support "
                   "scope. Use Diagnostics Bundle when reporting issues.");
        } else {
            bannerText =
                tr("Zarya stable release — experimental features are disabled by default. "
                   "Use Diagnostics Bundle when reporting issues.");
        }
    } else if (PackagingInfo::isReleaseCandidateBuild()) {
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
    setAccessibleName(bannerText);
    auto* label = new ZaryaBodyText(bannerText, this);
    auto* dismiss = new ZaryaActionButton(tr("Dismiss"), this);
    dismiss->setMinimumSize(96, 34);
    setFocusProxy(dismiss);
    connect(dismiss, &ZaryaActionButton::clicked, this, [this]() {
        AppSettings::instance().setDismissBetaBanner(true);
        hide();
        emit dismissed();
    });
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);
    layout->addWidget(label, 1);
    layout->addWidget(dismiss);
    setMinimumHeight(58);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            &BetaBannerWidget::applyTheme);
    applyTheme();
}

void BetaBannerWidget::applyTheme()
{
    update();
}

void BetaBannerWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    const ThemeTokens tokens = ThemeManager::instance().tokens();
    QPainter painter(this);
    painter.fillRect(rect(), tokens.warningSurface);
    painter.setPen(tokens.warning);
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());
}

} // namespace zarya
