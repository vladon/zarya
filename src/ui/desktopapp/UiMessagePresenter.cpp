#include "ui/desktopapp/UiMessagePresenter.h"

#include "ui/desktopapp/ZaryaControls.h"
#include "ui/desktopapp/ZaryaUiIntegration.h"
#include "ui/theme/ThemeManager.h"

#include "base/object_ptr.h"
#include "styles/style_layers.h"
#include "ui/qt_object_factory.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/scroll_area.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMetaObject>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <algorithm>
#include <rpl/rpl.h>

namespace zarya {
namespace {

constexpr int kDialogWidth = 520;
constexpr int kDialogMinimumWidth = 420;
constexpr int kContentMargin = 24;
constexpr int kContentSpacing = 10;
constexpr int kBodyMaximumHeight = 320;

QString toneLabel(UiMessageTone tone)
{
    switch (tone) {
    case UiMessageTone::Information:
        return UiMessagePresenter::tr("Information");
    case UiMessageTone::Warning:
        return UiMessagePresenter::tr("Attention");
    case UiMessageTone::Error:
        return UiMessagePresenter::tr("Error");
    }
    return {};
}

QColor toneColor(UiMessageTone tone)
{
    const ThemeTokens tokens = ThemeManager::instance().tokens();
    switch (tone) {
    case UiMessageTone::Information:
        return tokens.info;
    case UiMessageTone::Warning:
        return tokens.warning;
    case UiMessageTone::Error:
        return tokens.danger;
    }
    return tokens.textPrimary;
}

QMessageBox::Icon nativeIcon(UiMessageTone tone)
{
    switch (tone) {
    case UiMessageTone::Information:
        return QMessageBox::Information;
    case UiMessageTone::Warning:
        return QMessageBox::Warning;
    case UiMessageTone::Error:
        return QMessageBox::Critical;
    }
    return QMessageBox::NoIcon;
}

class MessageDialog final : public QDialog {
public:
    MessageDialog(
        QWidget* parent,
        const QString& title,
        const QString& text,
        UiMessageTone tone,
        bool confirmation,
        const QString& acceptText,
        bool destructive)
        : QDialog(parent)
        , m_tone(tone)
        , m_destructive(destructive)
    {
        setWindowTitle(title);
        setAccessibleName(title);
        setModal(true);
        setMinimumWidth(kDialogMinimumWidth);

        m_title = Ui::CreateChild<Ui::FlatLabel>(this, title, st::boxTitle);
        m_title->setAccessibleName(title);

        const QString semanticLabel = toneLabel(tone);
        m_toneLabel = Ui::CreateChild<Ui::FlatLabel>(this, semanticLabel, st::boxLabel);
        m_toneLabel->setAccessibleName(semanticLabel);
        updateToneColor();

        m_scroll = Ui::CreateChild<Ui::ScrollArea>(this, st::boxScroll);
        m_body = m_scroll->setOwnedWidget(
            object_ptr<Ui::FlatLabel>(m_scroll, text, st::defaultFlatLabel));
        m_body->setAccessibleName(text);
        m_body->setSelectable(true);

        auto* actionRow = new QWidget(this);
        auto* actionLayout = new QHBoxLayout(actionRow);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setSpacing(8);
        actionLayout->addStretch();

        if (confirmation) {
            const QString cancelText = UiMessagePresenter::tr("Cancel");
            auto cancel = makeZaryaButton(
                actionRow,
                cancelText,
                ZaryaButtonRole::Secondary);
            m_cancel = cancel.data();
            actionLayout->addWidget(cancel.release());
            m_cancel->setClickedCallback([this] {
                QMetaObject::invokeMethod(this, [this] { reject(); }, Qt::QueuedConnection);
            });
        }

        const QString primaryText = confirmation
            ? (acceptText.isEmpty() ? UiMessagePresenter::tr("Continue") : acceptText)
            : UiMessagePresenter::tr("Close");
        auto acceptButton = makeZaryaButton(
            actionRow,
            primaryText,
            destructive ? ZaryaButtonRole::Destructive : ZaryaButtonRole::Primary);
        m_accept = acceptButton.data();
        actionLayout->addWidget(acceptButton.release());
        m_accept->setClickedCallback([this] {
            QMetaObject::invokeMethod(this, [this] { QDialog::accept(); }, Qt::QueuedConnection);
        });

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(
            kContentMargin,
            kContentMargin,
            kContentMargin,
            kContentMargin);
        layout->setSpacing(kContentSpacing);
        layout->addWidget(m_title);
        layout->addWidget(m_toneLabel);
        layout->addWidget(m_scroll);
        layout->addSpacing(4);
        layout->addWidget(actionRow);

        connect(
            &ThemeManager::instance(),
            &ThemeManager::themeChanged,
            this,
            [this] { updateToneColor(); });

        resize(kDialogWidth, 240);
        relayoutContent();
        adjustSize();
        resize(kDialogWidth, height());
        if (m_destructive && m_cancel) {
            m_cancel->setFocus(Qt::OtherFocusReason);
        } else {
            m_accept->setFocus(Qt::OtherFocusReason);
        }
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QDialog::resizeEvent(event);
        relayoutContent();
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Escape) {
            reject();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (focusWidget() == m_cancel) {
                reject();
                return;
            }
            if (focusWidget() == m_accept || !m_destructive) {
                accept();
                return;
            }
        }
        QDialog::keyPressEvent(event);
    }

private:
    void updateToneColor()
    {
        m_toneLabel->setTextColorOverride(toneColor(m_tone));
    }

    void relayoutContent()
    {
        if (!m_title || !m_body || width() <= 0) {
            return;
        }
        const int contentWidth = std::max(width() - (2 * kContentMargin), 1);
        m_title->resizeToWidth(contentWidth);
        m_toneLabel->resizeToWidth(contentWidth);
        m_body->resizeToWidth(contentWidth);
        m_scroll->setFixedHeight(
            std::clamp(m_body->height(), m_body->st().style.lineHeight, kBodyMaximumHeight));
    }

    UiMessageTone m_tone;
    bool m_destructive = false;
    Ui::FlatLabel* m_title = nullptr;
    Ui::FlatLabel* m_toneLabel = nullptr;
    Ui::ScrollArea* m_scroll = nullptr;
    QPointer<Ui::FlatLabel> m_body;
    Ui::RoundButton* m_accept = nullptr;
    Ui::RoundButton* m_cancel = nullptr;
};

void showNativeMessage(
    QWidget* parent,
    const QString& title,
    const QString& text,
    UiMessageTone tone)
{
    QMessageBox box(nativeIcon(tone), title, text, QMessageBox::Ok, parent);
    box.exec();
}

} // namespace

void UiMessagePresenter::information(
    QWidget* parent,
    const QString& title,
    const QString& text)
{
    show(parent, title, text, UiMessageTone::Information);
}

void UiMessagePresenter::warning(
    QWidget* parent,
    const QString& title,
    const QString& text)
{
    show(parent, title, text, UiMessageTone::Warning);
}

void UiMessagePresenter::error(
    QWidget* parent,
    const QString& title,
    const QString& text)
{
    show(parent, title, text, UiMessageTone::Error);
}

bool UiMessagePresenter::confirm(
    QWidget* parent,
    const QString& title,
    const QString& text,
    const QString& acceptText,
    bool destructive)
{
    if (!desktopAppUiIntegrationsReady()) {
        const auto answer = QMessageBox::question(
            parent,
            title,
            text,
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);
        return answer == QMessageBox::Yes;
    }

    MessageDialog dialog(
        parent,
        title,
        text,
        destructive ? UiMessageTone::Warning : UiMessageTone::Information,
        true,
        acceptText,
        destructive);
    return dialog.exec() == QDialog::Accepted;
}

void UiMessagePresenter::show(
    QWidget* parent,
    const QString& title,
    const QString& text,
    UiMessageTone tone)
{
    if (!desktopAppUiIntegrationsReady()) {
        showNativeMessage(parent, title, text, tone);
        return;
    }

    MessageDialog dialog(parent, title, text, tone, false, {}, false);
    dialog.exec();
}

} // namespace zarya
