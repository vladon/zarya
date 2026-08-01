#pragma once

#include "domain/Profile.h"

#include <QDialog>
#include <QVector>

namespace zarya {

class ZaryaDialogActionRow;
class ZaryaTextArea;

class ImportVlessDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportVlessDialog(QWidget* parent = nullptr);

    QVector<Profile> importedProfiles() const;

private:
    void onImport();

    ZaryaTextArea* m_linksEdit = nullptr;
    ZaryaDialogActionRow* m_actions = nullptr;
    QVector<Profile> m_imported;
};

} // namespace zarya
