#include "ui/ImportVlessDialog.h"

#include "i18n/ZaryaTr.h"
#include "subscription/ShareLinkParser.h"

#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace zarya {

ImportVlessDialog::ImportVlessDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Import share links"));

    m_linksEdit = new QPlainTextEdit(this);
    m_linksEdit->setPlaceholderText(
        tr("Paste one vless://, vmess://, trojan://, or ss:// link per line…"));
    m_linksEdit->setMinimumHeight(160);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ImportVlessDialog::onImport);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_linksEdit);
    layout->addWidget(buttons);
    resize(560, 280);
}

QVector<Profile> ImportVlessDialog::importedProfiles() const
{
    return m_imported;
}

void ImportVlessDialog::onImport()
{
    const QStringList lines =
        m_linksEdit->toPlainText().split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                       Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        QMessageBox::warning(this, tr("Import"), tr("No links to import."));
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
        QMessageBox::warning(this, tr("Import failed"),
                             errors.join(QLatin1Char('\n')));
        return;
    }

    if (!errors.isEmpty()) {
        QMessageBox::warning(
            this, tr("Partial import"),
            ZaryaTr::plural("Imported %n profile(s). Some lines failed:", m_imported.size())
                + QStringLiteral("\n")
                + errors.join(QLatin1Char('\n')));
    }

    accept();
}

} // namespace zarya
