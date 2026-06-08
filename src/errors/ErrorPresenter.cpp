#include "errors/ErrorPresenter.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>

namespace zarya {

void ErrorPresenter::show(QWidget* parent, const AppError& error, bool includeCopyDiagnostics)
{
    QString text = error.message;
    if (!error.suggestedAction.isEmpty()) {
        text += QStringLiteral("\n\n") + error.suggestedAction;
    }

    QMessageBox box(parent);
    box.setIcon(error.severity == ErrorSeverity::Critical ? QMessageBox::Critical
                                                          : QMessageBox::Warning);
    box.setWindowTitle(error.title.isEmpty() ? QStringLiteral("Zarya") : error.title);
    box.setText(text);
    if (!error.details.isEmpty()) {
        box.setDetailedText(QStringLiteral("Code: %1\nArea: %2\n\n%3")
                                .arg(error.code, error.area, error.details));
    }
    box.setStandardButtons(QMessageBox::Ok);
    if (includeCopyDiagnostics) {
        QPushButton* copyButton = box.addButton(QStringLiteral("Copy details"),
                                                QMessageBox::ActionRole);
        QObject::connect(copyButton, &QPushButton::clicked, &box, [&error]() {
            const QString clipboard = QStringLiteral("[%1] %2\n%3\n%4")
                                        .arg(error.code, error.title, error.message, error.details);
            QApplication::clipboard()->setText(clipboard);
        });
    }
    box.exec();
}

} // namespace zarya
