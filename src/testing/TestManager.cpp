#include "testing/TestManager.h"

#include "storage/AppSettings.h"
#include "testing/RealDelayTester.h"
#include "testing/TcpPingTester.h"

#include <QDateTime>
#include <QMetaObject>
#include <QRunnable>
#include <QThreadPool>

namespace zarya {

namespace {

class TestRunnable : public QRunnable {
public:
    TestRunnable(TestManager* manager, Profile profile, TestMode mode,
                 std::atomic<bool>* cancelFlag)
        : m_manager(manager)
        , m_profile(std::move(profile))
        , m_mode(mode)
        , m_cancelFlag(cancelFlag)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        if (m_cancelFlag->load()) {
            finishCanceled();
            return;
        }

        Profile profile = m_profile;
        profile.lastTestStatus = TestStatus::Testing;
        profile.lastTestError.clear();
        profile.lastTestedAt = QDateTime::currentDateTimeUtc();
        m_manager->emitLogOnMainThread(
            QStringLiteral("Test started: %1").arg(profile.name));
        m_manager->emitTestStartedOnMainThread(profile.id);
        m_manager->emitProfileOnMainThread(profile);

        const AppSettings& settings = AppSettings::instance();
        const auto log = [this](const QString& line) { m_manager->emitLogOnMainThread(line); };

        const bool runTcp = m_mode == TestMode::TcpOnly
                            || (m_mode == TestMode::TcpThenRealDelay
                                && !settings.skipTcpBeforeRealDelay());
        const bool runDelay =
            m_mode == TestMode::RealDelayOnly || m_mode == TestMode::TcpThenRealDelay;

        TestStatus finalStatus = TestStatus::NeverTested;
        QString finalError;

        if (runTcp) {
            if (m_cancelFlag->load()) {
                finishCanceled();
                return;
            }
            m_manager->emitLogOnMainThread(
                QStringLiteral("TCP test: %1:%2").arg(profile.address).arg(profile.port));
            const TestResult tcpResult =
                TcpPingTester::run(profile, settings.tcpTestTimeoutMs());
            profile.lastTcpPingMs = tcpResult.tcpPingMs;
            if (tcpResult.status == TestStatus::Available) {
                m_manager->emitLogOnMainThread(
                    QStringLiteral("TCP OK: %1 ms").arg(tcpResult.tcpPingMs));
                finalStatus = TestStatus::Available;
            } else if (tcpResult.status == TestStatus::Timeout) {
                m_manager->emitLogOnMainThread(QStringLiteral("TCP timeout"));
                finalStatus = TestStatus::Timeout;
                finalError = tcpResult.errorMessage;
            } else {
                m_manager->emitLogOnMainThread(
                    QStringLiteral("TCP failed: %1").arg(tcpResult.errorMessage));
                finalStatus = TestStatus::Failed;
                finalError = tcpResult.errorMessage;
            }
        }

        if (runDelay) {
            if (m_cancelFlag->load()) {
                finishCanceled();
                return;
            }

            m_manager->emitLogOnMainThread(
                QStringLiteral("Real delay test started: %1").arg(profile.name));
            m_manager->emitLogOnMainThread(
                QStringLiteral("Test URL: %1").arg(settings.testUrl()));

            const TestResult delayResult = RealDelayTester::run(
                profile, settings.realDelayTimeoutMs(), settings.testUrl(), log);

            if (delayResult.status == TestStatus::Unsupported) {
                profile.lastRealDelayMs = -1;
                finalStatus = TestStatus::Unsupported;
                finalError = delayResult.errorMessage;
            } else if (delayResult.status == TestStatus::Available) {
                profile.lastRealDelayMs = delayResult.realDelayMs;
                finalStatus = TestStatus::Available;
                finalError.clear();
            } else if (delayResult.status == TestStatus::Timeout) {
                profile.lastRealDelayMs = -1;
                finalStatus = TestStatus::Timeout;
                finalError = delayResult.errorMessage;
            } else {
                profile.lastRealDelayMs = -1;
                finalStatus = TestStatus::Failed;
                finalError = delayResult.errorMessage;
            }

            if (!delayResult.errorMessage.isEmpty()
                && delayResult.status != TestStatus::Available) {
                m_manager->emitLogOnMainThread(
                    QStringLiteral("Test failed: %1").arg(delayResult.errorMessage));
            }
        } else if (m_mode == TestMode::TcpOnly && finalStatus == TestStatus::NeverTested) {
            finalStatus = profile.lastTcpPingMs >= 0 ? TestStatus::Available : TestStatus::Failed;
        }

        if (m_cancelFlag->load()) {
            finishCanceled();
            return;
        }

        profile.lastTestStatus = finalStatus;
        profile.lastTestError = finalError;
        profile.lastTestedAt = QDateTime::currentDateTimeUtc();
        m_manager->emitProfileOnMainThread(profile);
        m_manager->onJobFinished();
    }

private:
    void finishCanceled()
    {
        Profile profile = m_profile;
        profile.lastTestStatus = TestStatus::Canceled;
        profile.lastTestError = QStringLiteral("Test canceled.");
        profile.lastTestedAt = QDateTime::currentDateTimeUtc();
        m_manager->emitLogOnMainThread(QStringLiteral("Test canceled: %1").arg(profile.name));
        m_manager->emitProfileOnMainThread(profile);
        m_manager->onJobFinished();
    }

    TestManager* m_manager = nullptr;
    Profile m_profile;
    TestMode m_mode;
    std::atomic<bool>* m_cancelFlag = nullptr;
};

} // namespace

TestManager::TestManager(QObject* parent)
    : QObject(parent)
    , m_pool(QThreadPool::globalInstance())
{
}

bool TestManager::isBusy() const
{
    return m_busy;
}

void TestManager::startTests(const QVector<Profile>& profiles, TestMode mode)
{
    if (m_busy) {
        cancel();
    }

    QVector<Profile> queue;
    queue.reserve(profiles.size());
    for (const Profile& profile : profiles) {
        if (!profile.enabled || profile.deletedBySubscriptionUpdate) {
            continue;
        }
        queue.append(profile);
    }

    if (queue.isEmpty()) {
        emit logLine(QStringLiteral("No profiles selected for testing."));
        return;
    }

    m_cancelRequested.store(false);
    m_busy = true;
    m_totalProfiles = queue.size();
    m_finishedProfiles = 0;
    m_activeJobs.store(0);

    const int maxConcurrent = AppSettings::instance().maxConcurrentTests();
    m_pool->setMaxThreadCount(maxConcurrent);

    emit progressChanged(0, m_totalProfiles);
    emit logLine(QStringLiteral("Test batch started (%1 profiles).").arg(m_totalProfiles));

    for (const Profile& profile : queue) {
        m_activeJobs.fetch_add(1);
        m_pool->start(new TestRunnable(this, profile, mode, &m_cancelRequested));
    }
}

void TestManager::cancel()
{
    if (!m_busy) {
        return;
    }
    m_cancelRequested.store(true);
    emit logLine(QStringLiteral("Canceling tests…"));
}

void TestManager::onJobFinished()
{
    QMetaObject::invokeMethod(
        this,
        [this]() {
            m_activeJobs.fetch_sub(1);
            ++m_finishedProfiles;
            emit progressChanged(m_finishedProfiles, m_totalProfiles);
            emit logLine(QStringLiteral("Test progress: %1/%2")
                             .arg(m_finishedProfiles)
                             .arg(m_totalProfiles));
            checkAllFinished();
        },
        Qt::QueuedConnection);
}

void TestManager::checkAllFinished()
{
    if (m_activeJobs.load() > 0) {
        return;
    }
    if (!m_busy) {
        return;
    }
    m_busy = false;
    emit allFinished();
}

void TestManager::emitProfileOnMainThread(const Profile& profile)
{
    QMetaObject::invokeMethod(
        this, [this, profile]() { emit profileUpdated(profile); }, Qt::QueuedConnection);
}

void TestManager::emitTestStartedOnMainThread(const QString& profileId)
{
    QMetaObject::invokeMethod(
        this, [this, profileId]() { emit testStarted(profileId); }, Qt::QueuedConnection);
}

void TestManager::emitLogOnMainThread(const QString& line)
{
    QMetaObject::invokeMethod(
        this, [this, line]() { emit logLine(line); }, Qt::QueuedConnection);
}

} // namespace zarya
