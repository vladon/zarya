#pragma once

#include "domain/Profile.h"

#include <QObject>
#include <QThreadPool>
#include <QVector>
#include <atomic>

namespace zarya {

enum class TestMode {
    TcpOnly,
    RealDelayOnly,
    TcpThenRealDelay,
};

class TestManager : public QObject {
    Q_OBJECT

public:
    explicit TestManager(QObject* parent = nullptr);

    bool isBusy() const;
    void startTests(const QVector<Profile>& profiles, TestMode mode);
    void cancel();

    void emitProfileOnMainThread(const Profile& profile);
    void emitTestStartedOnMainThread(const QString& profileId);
    void emitLogOnMainThread(const QString& line);
    void onJobFinished();

signals:
    void testStarted(const QString& profileId);
    void profileUpdated(const Profile& profile);
    void progressChanged(int done, int total);
    void allFinished();
    void logLine(const QString& line);

private:
    void checkAllFinished();

    QThreadPool* m_pool = nullptr;
    std::atomic<bool> m_cancelRequested{false};
    std::atomic<int> m_activeJobs{0};
    int m_totalProfiles = 0;
    int m_finishedProfiles = 0;
    bool m_busy = false;
};

} // namespace zarya
