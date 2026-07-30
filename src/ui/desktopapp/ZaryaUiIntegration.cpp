#include "ui/desktopapp/ZaryaUiIntegration.h"

#include "storage/AppPaths.h"
#include "ui/desktopapp/ZaryaBaseIntegration.h"

#include "ui/effects/animations.h"
#include "ui/main_queue_processor.h"
#include "ui/style/style_core.h"
#include "ui/style/style_core_font.h"

#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>
#include <memory>

namespace zarya {
namespace {

ZaryaBaseIntegration* g_baseIntegration = nullptr;
ZaryaUiIntegration* g_uiIntegration = nullptr;
std::unique_ptr<Ui::MainQueueProcessor> g_mainQueueProcessor;
std::unique_ptr<Ui::Animations::Manager> g_animationsManager;
bool g_styleStarted = false;

} // namespace

void ZaryaUiIntegration::postponeCall(FnMut<void()>&& callable)
{
    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [fn = std::move(callable)]() mutable { fn(); },
        Qt::QueuedConnection);
}

void ZaryaUiIntegration::registerLeaveSubscription(not_null<QWidget*> widget)
{
    m_leaveSubscriptions.insert(widget.get(), 1);
}

void ZaryaUiIntegration::unregisterLeaveSubscription(not_null<QWidget*> widget)
{
    m_leaveSubscriptions.remove(widget.get());
}

QString ZaryaUiIntegration::emojiCacheFolder()
{
    const QString path = QDir(AppPaths::dataDir()).filePath(QStringLiteral("emoji-cache"));
    QDir().mkpath(path);
    return path;
}

QString ZaryaUiIntegration::openglCheckFilePath()
{
    return QDir(AppPaths::dataDir()).filePath(QStringLiteral("opengl-check"));
}

QString ZaryaUiIntegration::angleBackendFilePath()
{
    return QDir(AppPaths::dataDir()).filePath(QStringLiteral("angle-backend"));
}

void ZaryaUiIntegration::touchCounterIncrement()
{
    ++m_touchCounter;
}

int ZaryaUiIntegration::touchCounterNow()
{
    return m_touchCounter;
}

QString ZaryaUiIntegration::phraseContextCopyText()
{
    return QStringLiteral("Copy Text");
}
QString ZaryaUiIntegration::phraseContextCopyEmail()
{
    return QStringLiteral("Copy Email");
}
QString ZaryaUiIntegration::phraseContextCopyLink()
{
    return QStringLiteral("Copy Link");
}
QString ZaryaUiIntegration::phraseContextCopySelected()
{
    return QStringLiteral("Copy");
}
QString ZaryaUiIntegration::phraseFormattingTitle()
{
    return QStringLiteral("Formatting");
}
QString ZaryaUiIntegration::phraseFormattingLinkCreate()
{
    return QStringLiteral("Create Link");
}
QString ZaryaUiIntegration::phraseFormattingLinkEdit()
{
    return QStringLiteral("Edit Link");
}
QString ZaryaUiIntegration::phraseFormattingClear()
{
    return QStringLiteral("Clear Formatting");
}
QString ZaryaUiIntegration::phraseFormattingBold()
{
    return QStringLiteral("Bold");
}
QString ZaryaUiIntegration::phraseFormattingItalic()
{
    return QStringLiteral("Italic");
}
QString ZaryaUiIntegration::phraseFormattingUnderline()
{
    return QStringLiteral("Underline");
}
QString ZaryaUiIntegration::phraseFormattingStrikeOut()
{
    return QStringLiteral("Strikethrough");
}
QString ZaryaUiIntegration::phraseFormattingBlockquote()
{
    return QStringLiteral("Quote");
}
QString ZaryaUiIntegration::phraseFormattingMonospace()
{
    return QStringLiteral("Monospace");
}
QString ZaryaUiIntegration::phraseFormattingSpoiler()
{
    return QStringLiteral("Spoiler");
}
QString ZaryaUiIntegration::phraseFormattingDate()
{
    return QStringLiteral("Date");
}
QString ZaryaUiIntegration::phraseButtonOk()
{
    return QStringLiteral("OK");
}
QString ZaryaUiIntegration::phraseButtonClose()
{
    return QStringLiteral("Close");
}
QString ZaryaUiIntegration::phraseButtonCancel()
{
    return QStringLiteral("Cancel");
}
QString ZaryaUiIntegration::phrasePanelCloseWarning()
{
    return QStringLiteral("Close?");
}
QString ZaryaUiIntegration::phrasePanelCloseUnsaved()
{
    return QStringLiteral("Unsaved changes");
}
QString ZaryaUiIntegration::phrasePanelCloseAnyway()
{
    return QStringLiteral("Close anyway");
}
QString ZaryaUiIntegration::phraseBotSharePhone()
{
    return QStringLiteral("Share Phone");
}
QString ZaryaUiIntegration::phraseBotSharePhoneTitle()
{
    return QStringLiteral("Share Phone Number");
}
QString ZaryaUiIntegration::phraseBotSharePhoneConfirm()
{
    return QStringLiteral("Share");
}
QString ZaryaUiIntegration::phraseBotAllowWrite()
{
    return QStringLiteral("Allow Writing");
}
QString ZaryaUiIntegration::phraseBotAllowWriteTitle()
{
    return QStringLiteral("Allow Writing Messages");
}
QString ZaryaUiIntegration::phraseBotAllowWriteConfirm()
{
    return QStringLiteral("Allow");
}
QString ZaryaUiIntegration::phraseQuoteHeaderCopy()
{
    return QStringLiteral("Copy");
}
QString ZaryaUiIntegration::phraseMinimize()
{
    return QStringLiteral("Minimize");
}
QString ZaryaUiIntegration::phraseMaximize()
{
    return QStringLiteral("Maximize");
}
QString ZaryaUiIntegration::phraseRestore()
{
    return QStringLiteral("Restore");
}

void initDesktopAppUiIntegrations(int argc, char** argv)
{
    if (!g_baseIntegration) {
        g_baseIntegration = new ZaryaBaseIntegration(argc, argv);
        base::Integration::Set(g_baseIntegration);
    }
    if (!g_uiIntegration) {
        g_uiIntegration = new ZaryaUiIntegration();
        Ui::Integration::Set(g_uiIntegration);
    }
    // Required before any RippleButton / FlatLabel animation (Basic::start).
    if (!g_mainQueueProcessor) {
        g_mainQueueProcessor = std::make_unique<Ui::MainQueueProcessor>();
    }
    if (!g_animationsManager) {
        g_animationsManager = std::make_unique<Ui::Animations::Manager>();
    }
    if (!g_styleStarted) {
        style::internal::StartFonts();
        style::StartManager(style::Scale());
        g_styleStarted = true;
    }
}

bool desktopAppUiIntegrationsReady()
{
    return g_baseIntegration
        && g_uiIntegration
        && g_mainQueueProcessor
        && g_animationsManager
        && g_styleStarted;
}

} // namespace zarya
