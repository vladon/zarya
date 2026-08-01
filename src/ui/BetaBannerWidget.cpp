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
    auto* label = new ZaryaBodyText(bannerText, this);
    auto* dismiss = new ZaryaActionButton(tr("Dismiss"), this);
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
    const bool dark = ThemeManager::instance().effectiveIsDark();
    const QColor bg =
        dark ? QColor(QStringLiteral("#3d2e00")) : QColor(QStringLiteral("#fff8c5"));
    const QColor border = dark ? tokens.warning.darker(120) : QColor(QStringLiteral("#d4a72c"));
    QPainter painter(this);
    painter.fillRect(rect(), bg);
    painter.setPen(border);
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());
}

} // namespace zarya
