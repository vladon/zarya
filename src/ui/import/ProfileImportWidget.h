#pragma once

#include "domain/Profile.h"

#include <QWidget>

class QPlainTextEdit;
class QLabel;

namespace zarya {

struct ProfileImportStats {
    int vless = 0;
    int vmess = 0;
    int trojan = 0;
    int shadowsocks = 0;
    int unsupported = 0;
    int totalImported = 0;
};

class ProfileImportWidget : public QWidget {
    Q_OBJECT

public:
    explicit ProfileImportWidget(QWidget* parent = nullptr);

    QVector<Profile> importedProfiles() const;
    ProfileImportStats lastStats() const;
    void clear();

public slots:
    void parseLinks();

signals:
    void parseCompleted(const ProfileImportStats& stats);

private:
    QPlainTextEdit* m_linksEdit = nullptr;
    QLabel* m_statsLabel = nullptr;
    QVector<Profile> m_imported;
    ProfileImportStats m_stats;
};

} // namespace zarya
