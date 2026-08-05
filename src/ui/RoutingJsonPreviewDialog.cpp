#include "ui/RoutingJsonPreviewDialog.h"

#include "ui/desktopapp/ZaryaFormControls.h"

#include <QTimer>
#include <QVBoxLayout>

namespace zarya {

RoutingJsonPreviewDialog::RoutingJsonPreviewDialog(const QString& jsonText, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Xray Routing JSON Preview"));
    resize(640, 480);

    m_editor = new ZaryaTextArea(QString(), this, 380);
    m_editor->setReadOnly(true);
    m_editor->setAccessibleLabel(tr("Generated routing JSON"));
    m_editor->setText(jsonText);

    auto* closeButton = new ZaryaActionButton(tr("Close"), this);
    connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(m_editor);
    layout->addWidget(closeButton);
    QTimer::singleShot(0, m_editor, [this] {
        m_editor->setFocus(Qt::OtherFocusReason);
    });
}

} // namespace zarya
