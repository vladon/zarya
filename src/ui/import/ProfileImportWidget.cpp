#include "ui/import/ProfileImportWidget.h"

#include "domain/ProtocolType.h"
#include "subscription/ShareLinkParser.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace zarya {

ProfileImportWidget::ProfileImportWidget(QWidget* parent)
    : QWidget(parent)
{
    m_linksEdit = new QPlainTextEdit(this);
    m_linksEdit->setPlaceholderText(
        QStringLiteral("Paste vless://, vmess://, trojan://, or ss:// links here.\n"
                       "One link per line."));
    m_linksEdit->setMinimumHeight(140);

    m_statsLabel = new QLabel(this);
    m_statsLabel->setWordWrap(true);
    m_statsLabel->setText(QStringLiteral("Paste links to see parse summary."));

    connect(m_linksEdit, &QPlainTextEdit::textChanged, this, &ProfileImportWidget::parseLinks);

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
    m_linksEdit->clear();
    m_imported.clear();
    m_stats = {};
    m_statsLabel->setText(QStringLiteral("Paste links to see parse summary."));
}

void ProfileImportWidget::parseLinks()
{
    m_imported.clear();
    m_stats = {};

    const QStringList lines =
        m_linksEdit->toPlainText().split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
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
        default:
            break;
        }
    }
    m_stats.totalImported = m_imported.size();

    m_statsLabel->setText(
        QStringLiteral("Parsed: VLESS %1, VMess %2, Trojan %3, Shadowsocks %4, Unsupported %5")
            .arg(m_stats.vless)
            .arg(m_stats.vmess)
            .arg(m_stats.trojan)
            .arg(m_stats.shadowsocks)
            .arg(m_stats.unsupported));

    emit parseCompleted(m_stats);
}

} // namespace zarya
