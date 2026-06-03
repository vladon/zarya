#include "ui/RoutingJsonPreviewDialog.h"

#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace zarya {

RoutingJsonPreviewDialog::RoutingJsonPreviewDialog(const QString& jsonText, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Xray Routing JSON Preview"));
    resize(640, 480);

    m_editor = new QPlainTextEdit(this);
    m_editor->setReadOnly(true);
    m_editor->setPlainText(jsonText);

    auto* closeBox =
        new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    connect(closeBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_editor);
    layout->addWidget(closeBox);
}

} // namespace zarya
