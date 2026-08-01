#include "ui/theme/ThemeManager.h"

#include "storage/AppSettings.h"
#include "ui/desktopapp/ZaryaPalette.h"

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

namespace zarya {

namespace {

QString colorCss(const QColor& color)
{
    return color.name(QColor::HexRgb);
}

} // namespace

ThemeManager& ThemeManager::instance()
{
    static ThemeManager manager;
    return manager;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    m_tokens = lightTokens();
}

ThemeMode ThemeManager::mode() const
{
    return themeModeFromString(AppSettings::instance().themeMode());
}

void ThemeManager::setMode(ThemeMode mode)
{
    AppSettings::instance().setThemeMode(themeModeToString(mode));
    apply();
}

bool ThemeManager::effectiveIsDark() const
{
    return m_isDark;
}

ThemeTokens ThemeManager::tokens() const
{
    return m_tokens;
}

bool ThemeManager::resolveIsDark(ThemeMode mode) const
{
    if (mode == ThemeMode::Dark) {
        return true;
    }
    if (mode == ThemeMode::Light) {
        return false;
    }
    if (const QStyleHints* hints = QGuiApplication::styleHints()) {
        switch (hints->colorScheme()) {
        case Qt::ColorScheme::Dark:
            return true;
        case Qt::ColorScheme::Light:
            return false;
        case Qt::ColorScheme::Unknown:
        default:
            break;
        }
    }
    return false;
}

void ThemeManager::applyPalette(const ThemeTokens& tokens) const
{
    QPalette palette;
    palette.setColor(QPalette::Window, tokens.windowBg);
    palette.setColor(QPalette::WindowText, tokens.textPrimary);
    palette.setColor(QPalette::Base, tokens.surfaceBg);
    palette.setColor(QPalette::AlternateBase, tokens.panelBg);
    palette.setColor(QPalette::Text, tokens.textPrimary);
    palette.setColor(QPalette::Button, tokens.panelBg);
    palette.setColor(QPalette::ButtonText, tokens.textPrimary);
    palette.setColor(QPalette::BrightText, tokens.accentFg);
    palette.setColor(QPalette::Highlight, tokens.accent);
    palette.setColor(QPalette::HighlightedText, tokens.accentFg);
    palette.setColor(QPalette::Link, tokens.accent);
    palette.setColor(QPalette::LinkVisited, tokens.accentHover);
    palette.setColor(QPalette::ToolTipBase, tokens.surfaceBg);
    palette.setColor(QPalette::ToolTipText, tokens.textPrimary);
    palette.setColor(QPalette::PlaceholderText, tokens.textDisabled);

    palette.setColor(QPalette::Disabled, QPalette::WindowText, tokens.textDisabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, tokens.textDisabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, tokens.textDisabled);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, tokens.border);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, tokens.textSecondary);

    if (QApplication* app = qApp) {
        app->setPalette(palette);
    }
}

QString ThemeManager::buildStyleSheet(const ThemeTokens& tokens) const
{
    const QString windowBg = colorCss(tokens.windowBg);
    const QString surfaceBg = colorCss(tokens.surfaceBg);
    const QString panelBg = colorCss(tokens.panelBg);
    const QString textPrimary = colorCss(tokens.textPrimary);
    const QString textSecondary = colorCss(tokens.textSecondary);
    const QString accent = colorCss(tokens.accent);
    const QString accentFg = colorCss(tokens.accentFg);
    const QString border = colorCss(tokens.border);
    const QString radiusSm = QString::number(tokens.radiusSm);

    QString css = QStringLiteral(
        "QMainWindow, QDialog {"
        "  background-color: @windowBg;"
        "  color: @textPrimary;"
        "}"
        "QMenuBar {"
        "  background-color: @panelBg;"
        "  color: @textPrimary;"
        "  border-bottom: 1px solid @border;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: @accent;"
        "  color: @accentFg;"
        "}"
        "QMenu {"
        "  background-color: @surfaceBg;"
        "  color: @textPrimary;"
        "  border: 1px solid @border;"
        "}"
        "QMenu::item:selected {"
        "  background-color: @accent;"
        "  color: @accentFg;"
        "}"
        "QPlainTextEdit {"
        "  background-color: @surfaceBg;"
        "  color: @textPrimary;"
        "  border: 1px solid @border;"
        "  border-radius: @radiusSmpx;"
        "  padding: 4px 6px;"
        "  selection-background-color: @accent;"
        "  selection-color: @accentFg;"
        "}"
        "QPlainTextEdit:focus {"
        "  border-color: @accent;"
        "}"
        "QHeaderView::section {"
        "  background-color: @panelBg;"
        "  color: @textSecondary;"
        "  border: none;"
        "  border-right: 1px solid @border;"
        "  border-bottom: 1px solid @border;"
        "  padding: 6px;"
        "}"
        "QTableView {"
        "  background-color: @surfaceBg;"
        "  alternate-background-color: @panelBg;"
        "  color: @textPrimary;"
        "  border: 1px solid @border;"
        "  border-radius: @radiusSmpx;"
        "  gridline-color: @border;"
        "  selection-background-color: @accent;"
        "  selection-color: @accentFg;"
        "}"
        "QScrollBar:vertical {"
        "  background: @windowBg;"
        "  width: 12px;"
        "  margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: @border;"
        "  border-radius: 4px;"
        "  min-height: 24px;"
        "}"
        "QScrollBar:horizontal {"
        "  background: @windowBg;"
        "  height: 12px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: @border;"
        "  border-radius: 4px;"
        "  min-width: 24px;"
        "}"
        "QScrollBar::add-line, QScrollBar::sub-line {"
        "  width: 0;"
        "  height: 0;"
        "}"
        "QToolTip {"
        "  background-color: @surfaceBg;"
        "  color: @textPrimary;"
        "  border: 1px solid @border;"
        "  padding: 4px;"
        "}");

    css.replace(QStringLiteral("@windowBg"), windowBg);
    css.replace(QStringLiteral("@surfaceBg"), surfaceBg);
    css.replace(QStringLiteral("@panelBg"), panelBg);
    css.replace(QStringLiteral("@textPrimary"), textPrimary);
    css.replace(QStringLiteral("@textSecondary"), textSecondary);
    css.replace(QStringLiteral("@accentFg"), accentFg);
    css.replace(QStringLiteral("@accent"), accent);
    css.replace(QStringLiteral("@border"), border);
    css.replace(QStringLiteral("@radiusSm"), radiusSm);
    return css;
}

void ThemeManager::apply()
{
    QApplication* app = qApp;
    if (!app) {
        return;
    }

    if (!m_fusionApplied) {
        if (QStyle* style = QStyleFactory::create(QStringLiteral("Fusion"))) {
            app->setStyle(style);
        }
        m_fusionApplied = true;
    }

    if (!m_hintsConnected) {
        if (QStyleHints* hints = app->styleHints()) {
            connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
                if (mode() == ThemeMode::System) {
                    apply();
                }
            });
        }
        m_hintsConnected = true;
    }

    const ThemeMode currentMode = mode();
    m_isDark = resolveIsDark(currentMode);
    m_tokens = m_isDark ? darkTokens() : lightTokens();

    applyPalette(m_tokens);
    applyDesktopAppPalette(m_tokens);
    app->setStyleSheet(buildStyleSheet(m_tokens));
    emit themeChanged();
}

} // namespace zarya
