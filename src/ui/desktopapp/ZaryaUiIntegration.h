#pragma once

#include "ui/integration.h"

#include <QHash>
#include <QPointer>

class QWidget;

namespace zarya {

class ZaryaUiIntegration final : public Ui::Integration {
public:
    void postponeCall(FnMut<void()>&& callable) override;
    void registerLeaveSubscription(not_null<QWidget*> widget) override;
    void unregisterLeaveSubscription(not_null<QWidget*> widget) override;

    [[nodiscard]] QString emojiCacheFolder() override;
    [[nodiscard]] QString openglCheckFilePath() override;
    [[nodiscard]] QString angleBackendFilePath() override;

    void touchCounterIncrement() override;
    [[nodiscard]] int touchCounterNow() override;

    [[nodiscard]] QString phraseContextCopyText() override;
    [[nodiscard]] QString phraseContextCopyEmail() override;
    [[nodiscard]] QString phraseContextCopyLink() override;
    [[nodiscard]] QString phraseContextCopySelected() override;
    [[nodiscard]] QString phraseFormattingTitle() override;
    [[nodiscard]] QString phraseFormattingLinkCreate() override;
    [[nodiscard]] QString phraseFormattingLinkEdit() override;
    [[nodiscard]] QString phraseFormattingClear() override;
    [[nodiscard]] QString phraseFormattingBold() override;
    [[nodiscard]] QString phraseFormattingItalic() override;
    [[nodiscard]] QString phraseFormattingUnderline() override;
    [[nodiscard]] QString phraseFormattingStrikeOut() override;
    [[nodiscard]] QString phraseFormattingBlockquote() override;
    [[nodiscard]] QString phraseFormattingMonospace() override;
    [[nodiscard]] QString phraseFormattingSpoiler() override;
    [[nodiscard]] QString phraseFormattingDate() override;
    [[nodiscard]] QString phraseButtonOk() override;
    [[nodiscard]] QString phraseButtonClose() override;
    [[nodiscard]] QString phraseButtonCancel() override;
    [[nodiscard]] QString phrasePanelCloseWarning() override;
    [[nodiscard]] QString phrasePanelCloseUnsaved() override;
    [[nodiscard]] QString phrasePanelCloseAnyway() override;
    [[nodiscard]] QString phraseBotSharePhone() override;
    [[nodiscard]] QString phraseBotSharePhoneTitle() override;
    [[nodiscard]] QString phraseBotSharePhoneConfirm() override;
    [[nodiscard]] QString phraseBotAllowWrite() override;
    [[nodiscard]] QString phraseBotAllowWriteTitle() override;
    [[nodiscard]] QString phraseBotAllowWriteConfirm() override;
    [[nodiscard]] QString phraseQuoteHeaderCopy() override;
    [[nodiscard]] QString phraseMinimize() override;
    [[nodiscard]] QString phraseMaximize() override;
    [[nodiscard]] QString phraseRestore() override;

private:
    int m_touchCounter = 0;
    QHash<QWidget*, int> m_leaveSubscriptions;
};

void initDesktopAppUiIntegrations(int argc, char** argv);
[[nodiscard]] bool desktopAppUiIntegrationsReady();

} // namespace zarya
