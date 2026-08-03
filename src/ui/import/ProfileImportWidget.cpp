#include "ui/import/ProfileImportWidget.h"

#include "domain/ProtocolType.h"
#include "subscription/ShareLinkParser.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QAccessible>
#include <QAccessibleEvent>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace zarya {

ProfileImportWidget::ProfileImportWidget(QWidget* parent)
    : QWidget(parent)
{
    m_linksEdit = new ZaryaTextArea(
        tr("Paste vless://, vmess://, trojan://, ss://, hysteria2://, or "
           "wireguard:// links here.\n"
           "One link per line."),
        this,
        140);
    m_linksEdit->setAccessibleLabel(tr("Share links"));

    m_statsLabel = new ZaryaBodyText(
        tr("Paste links to see parse summary."),
        this);

    connect(m_linksEdit, &ZaryaTextArea::textChanged, this, &ProfileImportWidget::parseLinks);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_linksEdit);
    layout->addWidget(m_statsLabel);
}

QVector<Profile> ProfileImportWidget::importedProfiles() const
{
    return m_imported;
}

ProfileImportStats ProfileImportWidget::lastStats() const
{
    return m_stats;
}

void ProfileImportWidget::clear()
{
    const QSignalBlocker blocker(m_linksEdit);
    m_linksEdit->clear();
    m_imported.clear();
    m_stats = {};
    updateSummary(tr("Paste links to see parse summary."), false);
}

void ProfileImportWidget::parseLinks()
{
    m_imported.clear();
    m_stats = {};

    const QStringList lines =
        m_linksEdit->text().split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                  Qt::SkipEmptyParts);
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (!ShareLinkParser::isSupportedScheme(line)) {
            ++m_stats.unsupported;
            continue;
        }
        const ShareLinkParseResult result = ShareLinkParser::parse(line);
        if (!result.ok) {
            ++m_stats.unsupported;
            continue;
        }
        m_imported.append(result.profile);
        switch (result.profile.protocol) {
        case ProtocolType::Vless:
            ++m_stats.vless;
            break;
        case ProtocolType::Vmess:
            ++m_stats.vmess;
            break;
        case ProtocolType::Trojan:
            ++m_stats.trojan;
            break;
        case ProtocolType::Shadowsocks:
            ++m_stats.shadowsocks;
            break;
        case ProtocolType::Hysteria2:
            ++m_stats.hysteria2;
            break;
        case ProtocolType::WireGuard:
            ++m_stats.wireguard;
            break;
        default:
            break;
        }
    }
    m_stats.totalImported = m_imported.size();

    const QString summary =
        tr("Parsed: VLESS %1, VMess %2, Trojan %3, Shadowsocks %4, Hysteria2 %5, WireGuard %6, "
           "Unsupported %7")
            .arg(m_stats.vless)
            .arg(m_stats.vmess)
            .arg(m_stats.trojan)
            .arg(m_stats.shadowsocks)
            .arg(m_stats.hysteria2)
            .arg(m_stats.wireguard)
            .arg(m_stats.unsupported);
    updateSummary(summary, true);

    emit parseCompleted(m_stats);
}

void ProfileImportWidget::updateSummary(const QString& text, bool announce)
{
    m_statsLabel->setText(text);
    if (announce && isVisible()) {
        QAccessibleAnnouncementEvent event(m_statsLabel, text);
        event.setPoliteness(QAccessible::AnnouncementPoliteness::Polite);
        QAccessible::updateAccessibility(&event);
    }
}

} // namespace zarya
