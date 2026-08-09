#include "platform/ManagedCoreOrphanCleanup.h"

#include "storage/AppPaths.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace zarya {
namespace {

QStringList managedCoreExecutablePaths()
{
    QStringList paths;
#if defined(Q_OS_WIN)
    paths.append(QDir(AppPaths::applicationDir())
                     .filePath(QStringLiteral("zarya-core-test-worker.exe")));
#else
    paths.append(QDir(AppPaths::applicationDir())
                     .filePath(QStringLiteral("zarya-core-test-worker")));
#endif
    return paths;
}

QString normalizeExecutablePath(const QString& path)
{
    if (path.isEmpty()) {
        return {};
    }
    const QFileInfo info(path);
    const QString absolute = info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
    return QDir::toNativeSeparators(absolute).toLower();
}

#if defined(Q_OS_WIN)

QString processImagePath(DWORD pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return {};
    }

    WCHAR buffer[MAX_PATH * 4];
    DWORD size = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
    QString path;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size)) {
        path = QString::fromWCharArray(buffer, static_cast<int>(size));
    }
    CloseHandle(process);
    return path;
}

template <typename Callback>
void forEachMatchingManagedCore(const QStringList& targets, Callback&& callback)
{
    QSet<QString> normalizedTargets;
    for (const QString& target : targets) {
        const QString normalized = normalizeExecutablePath(target);
        if (!normalized.isEmpty()) {
            normalizedTargets.insert(normalized);
        }
    }
    if (normalizedTargets.isEmpty()) {
        return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (entry.th32ProcessID == 0 || entry.th32ProcessID == GetCurrentProcessId()) {
                continue;
            }
            const QString imagePath = processImagePath(entry.th32ProcessID);
            const QString normalized = normalizeExecutablePath(imagePath);
            if (normalized.isEmpty() || !normalizedTargets.contains(normalized)) {
                continue;
            }
            callback(entry.th32ProcessID, imagePath);
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
}

bool terminatePid(DWORD pid)
{
    HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
    if (!process) {
        return false;
    }
    const BOOL ok = TerminateProcess(process, 1);
    if (ok) {
        WaitForSingleObject(process, 3000);
    }
    CloseHandle(process);
    return ok == TRUE;
}

#endif

} // namespace

bool hasOrphanedManagedCores()
{
#if defined(Q_OS_WIN)
    bool found = false;
    forEachMatchingManagedCore(managedCoreExecutablePaths(),
                               [&](DWORD, const QString&) { found = true; });
    return found;
#else
    return false;
#endif
}

ManagedCoreOrphanCleanupResult terminateOrphanedManagedCores()
{
    ManagedCoreOrphanCleanupResult result;
#if defined(Q_OS_WIN)
    forEachMatchingManagedCore(managedCoreExecutablePaths(), [&](DWORD pid, const QString& path) {
        if (terminatePid(pid)) {
            ++result.terminatedCount;
            result.details.append(
                QStringLiteral("Terminated leftover core process pid=%1 (%2)").arg(pid).arg(path));
        } else {
            result.details.append(
                QStringLiteral("Failed to terminate leftover core process pid=%1 (%2)")
                    .arg(pid)
                    .arg(path));
        }
    });
#else
    Q_UNUSED(result);
#endif
    return result;
}

} // namespace zarya
