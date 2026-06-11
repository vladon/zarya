#include "helper/WindowsServiceRunner.h"

#include "helper/HelperApplication.h"

#include <QVector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace zarya {

namespace {

#ifdef Q_OS_WIN
HelperApplication* g_application = nullptr;
SERVICE_STATUS g_serviceStatus{};
SERVICE_STATUS_HANDLE g_statusHandle = nullptr;

void reportServiceStatus(DWORD currentState, DWORD win32ExitCode = NO_ERROR, DWORD waitHint = 0)
{
    static DWORD checkpoint = 1;
    g_serviceStatus.dwCurrentState = currentState;
    g_serviceStatus.dwWin32ExitCode = win32ExitCode;
    g_serviceStatus.dwWaitHint = waitHint;
    g_serviceStatus.dwCheckPoint =
        (currentState == SERVICE_START_PENDING || currentState == SERVICE_STOP_PENDING)
            ? checkpoint++
            : 0;
    if (g_statusHandle) {
        SetServiceStatus(g_statusHandle, &g_serviceStatus);
    }
}

DWORD WINAPI serviceControlHandler(DWORD control, DWORD, LPVOID, LPVOID)
{
    switch (control) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        reportServiceStatus(SERVICE_STOP_PENDING);
        if (g_application) {
            g_application->requestServiceStop();
        }
        return NO_ERROR;
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

void WINAPI serviceMain(DWORD argc, LPWSTR* argv)
{
    g_statusHandle = RegisterServiceCtrlHandlerExW(L"ZaryaHelper", serviceControlHandler, nullptr);
    if (!g_statusHandle) {
        return;
    }

    g_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    reportServiceStatus(SERVICE_START_PENDING);

    int fakeArgc = 0;
    QVector<QByteArray> storage;
    QVector<char*> fakeArgv;
    fakeArgv.push_back(const_cast<char*>("zarya-helper"));
    ++fakeArgc;
    for (DWORD index = 0; index < argc; ++index) {
        storage.append(QString::fromWCharArray(argv[index]).toUtf8());
        fakeArgv.push_back(storage.last().data());
        ++fakeArgc;
    }
    bool hasServiceFlag = false;
    for (const QByteArray& item : storage) {
        if (item == "--service") {
            hasServiceFlag = true;
            break;
        }
    }
    if (!hasServiceFlag) {
        storage.append(QByteArray("--service"));
        fakeArgv.push_back(storage.last().data());
        ++fakeArgc;
    }

    HelperApplication application(fakeArgc, fakeArgv.data());
    g_application = &application;
    application.setRunningAsWindowsService(true);

    QString error;
    if (!application.prepareServiceMode(&error)) {
        reportServiceStatus(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR);
        g_application = nullptr;
        return;
    }

    reportServiceStatus(SERVICE_RUNNING);
    const int code = application.runServiceEventLoop();
    reportServiceStatus(SERVICE_STOPPED);
    g_application = nullptr;
    Q_UNUSED(code);
}
#endif

} // namespace

bool WindowsServiceRunner::shouldRunAsService(int argc, char** argv)
{
#ifdef Q_OS_WIN
    for (int index = 1; index < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == QStringLiteral("--service")) {
            return true;
        }
    }
#endif
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    return false;
}

int WindowsServiceRunner::runAsService(int argc, char** argv)
{
#ifdef Q_OS_WIN
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    SERVICE_TABLE_ENTRYW table[] = {{const_cast<LPWSTR>(L"ZaryaHelper"), serviceMain},
                                    {nullptr, nullptr}};
    if (!StartServiceCtrlDispatcherW(table)) {
        const DWORD error = GetLastError();
        if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            return -1;
        }
        return static_cast<int>(error);
    }
    return 0;
#else
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    return -1;
#endif
}

} // namespace zarya
