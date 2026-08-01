#include "ui/ImportVlessDialog.h"

#include "i18n/ZaryaTr.h"
#include "subscription/ShareLinkParser.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QRegularExpression>
#include <QVBoxLayout>

namespace zarya {

ImportVlessDialog::ImportVlessDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Import share links"));
    setAccessibleName(tr("Import share links"));

    m_linksEdit = new ZaryaTextArea(
        tr("Paste one vless://, vmess://, trojan://, ss://, hysteria2://, or wireguard:// "
           "link per line…"),
        this,
        160);

    m_actions = new ZaryaDialogActionRow(
        tr("Import"),
        tr("Cancel"),
        this);
    connect(m_actions, &ZaryaDialogActionRow::accepted, this, &ImportVlessDialog::onImport);
    connect(m_actions, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(m_linksEdit);
    layout->addWidget(m_actions);
    resize(620, 340);
    m_actions->focusAccept();
}

QVector<Profile> ImportVlessDialog::importedProfiles() const
{
    return m_imported;
}

void ImportVlessDialog::onImport()
{
    const QStringList lines =
        m_linksEdit->text().split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                  Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        UiMessagePresenter::warning(this, tr("Import"), tr("No links to import."));
        return;
    }

    m_imported.clear();
    QStringList errors;
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const ShareLinkParseResult result = ShareLinkParser::parse(line);
        if (result.ok) {
            m_imported.append(result.profile);
        } else {
            errors.append(result.error);
        }
    }

    if (m_imported.isEmpty()) {
        UiMessagePresenter::warning(
            this,
            tr("Import failed"),
            errors.join(QLatin1Char('\n')));
        return;
    }

    if (!errors.isEmpty()) {
        UiMessagePresenter::warning(
            this, tr("Partial import"),
            ZaryaTr::plural("Imported %n profile(s). Some lines failed:", m_imported.size())
                + QStringLiteral("\n")
                + errors.join(QLatin1Char('\n')));
    }

    accept();
}

} // namespace zarya
