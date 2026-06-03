#include "ui/ImportVlessDialog.h"

#include "import/VlessUriParser.h"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace zarya {

ImportVlessDialog::ImportVlessDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Import VLESS links"));

    m_linksEdit = new QPlainTextEdit(this);
    m_linksEdit->setPlaceholderText(
        QStringLiteral("Paste one vless:// link per line…"));
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
    const QVector<VlessParseResult> results = VlessUriParser::parseMany(m_linksEdit->toPlainText());
    if (results.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Import"), QStringLiteral("No links to import."));
        return;
    }

    m_imported.clear();
    QStringList errors;
    for (const VlessParseResult& result : results) {
        if (result.success) {
            m_imported.append(result.profile);
        } else {
            errors.append(result.errorMessage);
        }
    }

    if (m_imported.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Import failed"),
                             errors.join(QLatin1Char('\n')));
        return;
    }

    if (!errors.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Partial import"),
                             QStringLiteral("Imported %1 profile(s). Some lines failed:\n%2")
                                 .arg(m_imported.size())
                                 .arg(errors.join(QLatin1Char('\n'))));
    }

    accept();
}

} // namespace zarya
