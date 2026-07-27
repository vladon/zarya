#include "ui/BetaBannerWidget.h"

#include "packaging/PackagingInfo.h"
#include "storage/AppSettings.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/ThemeTokens.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

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

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            &BetaBannerWidget::applyTheme);
    applyTheme();
}

void BetaBannerWidget::applyTheme()
{
    const ThemeTokens tokens = ThemeManager::instance().tokens();
    const bool dark = ThemeManager::instance().effectiveIsDark();
    const QColor bg =
        dark ? QColor(QStringLiteral("#3d2e00")) : QColor(QStringLiteral("#fff8c5"));
    const QColor border = dark ? tokens.warning.darker(120) : QColor(QStringLiteral("#d4a72c"));
    setStyleSheet(QStringLiteral("background:%1; border-bottom:1px solid %2;")
                      .arg(bg.name(QColor::HexRgb), border.name(QColor::HexRgb)));
    if (m_label) {
        m_label->setStyleSheet(QStringLiteral("color:%1; background:transparent;")
                                   .arg(tokens.warning.name(QColor::HexRgb)));
    }
}

} // namespace zarya
