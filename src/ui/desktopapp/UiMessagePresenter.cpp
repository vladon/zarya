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
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <algorithm>
#include <utility>
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

ZaryaButtonRole buttonRole(UiMessageActionRole role)
{
    switch (role) {
    case UiMessageActionRole::Primary:
        return ZaryaButtonRole::Primary;
    case UiMessageActionRole::Secondary:
        return ZaryaButtonRole::Secondary;
    case UiMessageActionRole::Destructive:
        return ZaryaButtonRole::Destructive;
    }
    return ZaryaButtonRole::Secondary;
}

QMessageBox::ButtonRole nativeButtonRole(const UiMessageAction& action)
{
    if (action.isCancel) {
        return QMessageBox::RejectRole;
    }
    switch (action.role) {
    case UiMessageActionRole::Primary:
        return QMessageBox::AcceptRole;
    case UiMessageActionRole::Secondary:
        return QMessageBox::ActionRole;
    case UiMessageActionRole::Destructive:
        return QMessageBox::DestructiveRole;
    }
    return QMessageBox::ActionRole;
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

class ActionMessageDialog final : public QDialog {
public:
    ActionMessageDialog(
        QWidget* parent,
        const QString& title,
        const QString& text,
        UiMessageTone tone,
        QVector<UiMessageAction> actions)
        : QDialog(parent)
        , m_tone(tone)
        , m_actions(std::move(actions))
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

        for (const UiMessageAction& action : m_actions) {
            auto button = makeZaryaButton(actionRow, action.text, buttonRole(action.role));
            auto* pointer = button.data();
            m_buttons.push_back(pointer);
            actionLayout->addWidget(button.release());
            if (action.isDefault) {
                m_defaultButton = pointer;
            }
            if (action.isCancel) {
                m_cancelButton = pointer;
            }
            const QString id = action.id;
            const bool cancel = action.isCancel;
            pointer->setClickedCallback([this, id, cancel] {
                QMetaObject::invokeMethod(this, [this, id, cancel] {
                    finish(id, cancel);
                }, Qt::QueuedConnection);
            });
        }

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

        const bool destructive = std::any_of(
            m_actions.cbegin(),
            m_actions.cend(),
            [](const UiMessageAction& action) {
                return action.role == UiMessageActionRole::Destructive;
            });
        if (destructive && m_cancelButton) {
            m_cancelButton->setFocus(Qt::OtherFocusReason);
        } else if (m_defaultButton) {
            m_defaultButton->setFocus(Qt::OtherFocusReason);
        } else if (!m_buttons.empty()) {
            m_buttons.back()->setFocus(Qt::OtherFocusReason);
        }
    }

    [[nodiscard]] QString selectedActionId() const
    {
        return m_selectedActionId;
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
            const auto cancel = std::find_if(
                m_actions.cbegin(),
                m_actions.cend(),
                [](const UiMessageAction& action) { return action.isCancel; });
            finish(cancel != m_actions.cend() ? cancel->id : QString(), true);
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            for (int i = 0; i < m_buttons.size(); ++i) {
                if (focusWidget() == m_buttons.at(i)) {
                    finish(m_actions.at(i).id, m_actions.at(i).isCancel);
                    return;
                }
            }
            const auto action = std::find_if(
                m_actions.cbegin(),
                m_actions.cend(),
                [](const UiMessageAction& candidate) { return candidate.isDefault; });
            if (action != m_actions.cend()) {
                finish(action->id, action->isCancel);
                return;
            }
        }
        QDialog::keyPressEvent(event);
    }

private:
    void finish(const QString& id, bool cancel)
    {
        m_selectedActionId = id;
        done(cancel ? QDialog::Rejected : QDialog::Accepted);
    }

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
    QVector<UiMessageAction> m_actions;
    QVector<Ui::RoundButton*> m_buttons;
    QString m_selectedActionId;
    Ui::FlatLabel* m_title = nullptr;
    Ui::FlatLabel* m_toneLabel = nullptr;
    Ui::ScrollArea* m_scroll = nullptr;
    QPointer<Ui::FlatLabel> m_body;
    Ui::RoundButton* m_defaultButton = nullptr;
    Ui::RoundButton* m_cancelButton = nullptr;
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

QString chooseNativeAction(
    QWidget* parent,
    const QString& title,
    const QString& text,
    UiMessageTone tone,
    const QVector<UiMessageAction>& actions)
{
    QMessageBox box(nativeIcon(tone), title, text, QMessageBox::NoButton, parent);
    QVector<QAbstractButton*> buttons;
    buttons.reserve(actions.size());
    for (const UiMessageAction& action : actions) {
        auto* button = box.addButton(action.text, nativeButtonRole(action));
        buttons.push_back(button);
        if (action.isDefault) {
            box.setDefaultButton(button);
        }
        if (action.isCancel) {
            box.setEscapeButton(button);
        }
    }
    box.exec();
    const int index = buttons.indexOf(box.clickedButton());
    return index >= 0 ? actions.at(index).id : QString();
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

QString UiMessagePresenter::choose(
    QWidget* parent,
    const QString& title,
    const QString& text,
    UiMessageTone tone,
    const QVector<UiMessageAction>& actions)
{
    if (actions.isEmpty()) {
        return {};
    }
    if (!desktopAppUiIntegrationsReady()) {
        return chooseNativeAction(parent, title, text, tone, actions);
    }

    ActionMessageDialog dialog(parent, title, text, tone, actions);
    dialog.exec();
    return dialog.selectedActionId();
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
