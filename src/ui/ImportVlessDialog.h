#pragma once

#include "domain/Profile.h"

#include <QDialog>
#include <QVector>

class QPlainTextEdit;

namespace zarya {

class ImportVlessDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportVlessDialog(QWidget* parent = nullptr);

    QVector<Profile> importedProfiles() const;

private:
    void onImport();

    QPlainTextEdit* m_linksEdit = nullptr;
    QVector<Profile> m_imported;
};

} // namespace zarya
