#pragma once

#include "domain/Profile.h"

#include <QWidget>

namespace zarya {

class ZaryaBodyText;
class ZaryaTextArea;

struct ProfileImportStats {
    int vless = 0;
    int vmess = 0;
    int trojan = 0;
    int shadowsocks = 0;
    int hysteria2 = 0;
    int wireguard = 0;
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

public Q_SLOTS:
    void parseLinks();

Q_SIGNALS:
    void parseCompleted(const ProfileImportStats& stats);

private:
    ZaryaTextArea* m_linksEdit = nullptr;
    ZaryaBodyText* m_statsLabel = nullptr;
    QVector<Profile> m_imported;
    ProfileImportStats m_stats;
};

} // namespace zarya
