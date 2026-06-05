#include "killswitch/linux/LinuxNftKillSwitchManager.h"

#include "storage/AppPaths.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>

namespace zarya {

namespace {

QString joinIpSet(const QStringList& ips)
{
    if (ips.isEmpty()) {
        return {};
    }
    QStringList quoted;
    quoted.reserve(ips.size());
    for (const QString& ip : ips) {
        quoted.append(ip);
    }
    return quoted.join(QStringLiteral(", "));
}

} // namespace

QString LinuxNftKillSwitchManager::backendId() const
{
    return QStringLiteral("linux-nft");
}

QString LinuxNftKillSwitchManager::displayName() const
{
    return QStringLiteral("Linux nftables PoC");
}

KillSwitchState LinuxNftKillSwitchManager::checkSupport(bool privileged) const
{
    KillSwitchState state;
    state.backend = backendId();
    state.privileged = privileged;

    const QString nftPath = QStandardPaths::findExecutable(QStringLiteral("nft"));
    if (nftPath.isEmpty()) {
        state.status = KillSwitchStatus::Unsupported;
        state.supported = false;
        state.lastError = QStringLiteral("nft command not found in PATH.");
        return state;
    }
    if (!privileged) {
        state.status = KillSwitchStatus::Unsupported;
        state.supported = false;
        state.lastError =
            QStringLiteral("Linux nftables kill switch requires root privileges (sudo helper).");
        return state;
    }

    state.status = KillSwitchStatus::Disabled;
    state.supported = true;
    return state;
}

QString LinuxNftKillSwitchManager::rulesFilePath() const
{
    return AppPaths::killSwitchRulesFilePath();
}

QString LinuxNftKillSwitchManager::buildRulesFile(const KillSwitchRuleSet& rules,
                                                  QString* errorMessage) const
{
    AppPaths::ensureKillSwitchDir();
    const QString path = rulesFilePath();

    QString content;
    content += QStringLiteral("table inet zarya {\n");
    content += QStringLiteral("  chain output {\n");
    content += QStringLiteral("    type filter hook output priority 0; policy accept;\n\n");

    if (rules.allowLoopback) {
        content += QStringLiteral("    oifname \"lo\" accept\n");
        content += QStringLiteral("    ip daddr 127.0.0.0/8 accept\n");
        content += QStringLiteral("    ip6 daddr ::1 accept\n\n");
    }

    if (rules.allowLan) {
        content += QStringLiteral(
            "    ip daddr { 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16 } accept\n");
        content += QStringLiteral("    ip6 daddr { fc00::/7, fe80::/10 } accept\n\n");
    }

    content += QStringLiteral("    oifname \"%1\" accept\n\n").arg(rules.tunInterfaceName);

    const QString ipv4Set = joinIpSet(rules.proxyServerIpv4);
    if (!ipv4Set.isEmpty()) {
        content += QStringLiteral("    ip daddr { %1 } tcp dport %2 accept\n")
                       .arg(ipv4Set)
                       .arg(rules.proxyServerPort);
    }
    const QString ipv6Set = joinIpSet(rules.proxyServerIpv6);
    if (!ipv6Set.isEmpty()) {
        content += QStringLiteral("    ip6 daddr { %1 } tcp dport %2 accept\n")
                       .arg(ipv6Set)
                       .arg(rules.proxyServerPort);
    }
    if (!ipv4Set.isEmpty() || !ipv6Set.isEmpty()) {
        content += QStringLiteral("\n");
    }

    if (rules.blockDirectDns) {
        content += QStringLiteral("    udp dport 53 reject\n");
        content += QStringLiteral("    tcp dport 53 reject\n\n");
    }

    content += QStringLiteral("    reject\n");
    content += QStringLiteral("  }\n");
    content += QStringLiteral("}\n");

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return {};
    }
    file.write(content.toUtf8());
    return path;
}

bool LinuxNftKillSwitchManager::runNft(const QStringList& arguments, QString* output,
                                       QString* errorMessage) const
{
    QProcess process;
    process.setProgram(QStandardPaths::findExecutable(QStringLiteral("nft")));
    process.setArguments(arguments);
    process.start();
    if (!process.waitForFinished(15000)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("nft command timed out.");
        }
        return false;
    }
    const QString stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    const QString stderrText = QString::fromUtf8(process.readAllStandardError());
    if (output) {
        *output = stdoutText;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMessage) {
            *errorMessage = stderrText.trimmed().isEmpty() ? stdoutText.trimmed() : stderrText.trimmed();
        }
        return false;
    }
    return true;
}

bool LinuxNftKillSwitchManager::enable(const KillSwitchRuleSet& rules, QString* errorMessage)
{
    if (rules.proxyServerIpv4.isEmpty() && rules.proxyServerIpv6.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No proxy server IP addresses provided for kill switch.");
        }
        return false;
    }

    const QString rulesPath = buildRulesFile(rules, errorMessage);
    if (rulesPath.isEmpty()) {
        return false;
    }

    QString output;
    if (!runNft({QStringLiteral("-f"), rulesPath}, &output, errorMessage)) {
        return false;
    }
    runNft({QStringLiteral("list"), QStringLiteral("table"), QStringLiteral("inet"),
            QStringLiteral("zarya")},
           &output, nullptr);
    return true;
}

bool LinuxNftKillSwitchManager::disable(QString* errorMessage)
{
    QString output;
    if (!runNft({QStringLiteral("delete"), QStringLiteral("table"), QStringLiteral("inet"),
                 QStringLiteral("zarya")},
                &output, errorMessage)) {
        const QString message = errorMessage ? *errorMessage : QString();
        if (message.contains(QStringLiteral("No such file"), Qt::CaseInsensitive)
            || message.contains(QStringLiteral("does not exist"), Qt::CaseInsensitive)) {
            return true;
        }
        return false;
    }
    return true;
}

QString LinuxNftKillSwitchManager::recoveryInstructions() const
{
    return QStringLiteral(
        "Linux nftables recovery:\n\n"
        "  sudo nft list tables\n"
        "  sudo nft list table inet zarya\n"
        "  sudo nft delete table inet zarya\n\n"
        "Zarya only manages table inet zarya and never runs 'nft flush ruleset'.");
}

} // namespace zarya
