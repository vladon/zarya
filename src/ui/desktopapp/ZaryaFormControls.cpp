#include "ui/desktopapp/ZaryaFormControls.h"

#include "ui/desktopapp/ZaryaControls.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/ThemeTokens.h"

#include "base/object_ptr.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "ui/qt_object_factory.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/fields/number_input.h"
#include "ui/widgets/fields/password_input.h"
#include "ui/widgets/labels.h"

#include <QAccessible>
#include <QHBoxLayout>
#include <QMetaObject>
#include <QResizeEvent>
#include <QTextEdit>
#include <QVBoxLayout>
#include <algorithm>
#include <cstdlib>
#include <rpl/rpl.h>

namespace zarya {
namespace {

class AccessibleTextAreaField final : public Ui::InputField {
public:
    using Ui::InputField::InputField;

    [[nodiscard]] Ui::AccessibilityState accessibilityState() const override
    {
        auto result = Ui::InputField::accessibilityState();
        result.readOnly = rawTextEdit()->isReadOnly();
        return result;
    }

    void setReadOnly(bool readOnly)
    {
        if (rawTextEdit()->isReadOnly() == readOnly) {
            return;
        }
        rawTextEdit()->setReadOnly(readOnly);
        accessibilityStateChanged({.readOnly = true});
    }
};

int rpWidgetHeight(QWidget* widget)
{
    return std::max({widget->height(), widget->minimumHeight(), 1});
}

void preserveRpWidgetSize(QWidget* widget)
{
    widget->setMinimumSize(
        std::max(widget->width(), widget->minimumWidth()),
        rpWidgetHeight(widget));
}

void fitRpWidget(QWidget* widget, int width)
{
    static_cast<Ui::RpWidget*>(widget)->resizeToWidth(std::max(width, 0));
}

} // namespace

ZaryaActionButton::ZaryaActionButton(const QString& text, QWidget* parent)
    : ZaryaActionButton(text, parent, ZaryaButtonRole::Secondary)
{
}

ZaryaActionButton::ZaryaActionButton(
    const QString& text,
    QWidget* parent,
    ZaryaButtonRole role)
    : QWidget(parent)
{
    auto button = makeZaryaButton(this, text, role);
    m_button = button.data();
    setFocusProxy(m_button);
    static_cast<Ui::RoundButton*>(m_button)->setClickedCallback([this] {
        QMetaObject::invokeMethod(this, [this] { Q_EMIT clicked(); }, Qt::QueuedConnection);
    });
    preserveRpWidgetSize(m_button);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->addWidget(button.release());
    m_button->show();
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setMinimumSize(m_button->minimumSize());
}

void ZaryaActionButton::setText(const QString& text)
{
    auto* button = static_cast<Ui::RoundButton*>(m_button);
    button->setText(rpl::single(text));
    button->setAccessibleName(text);
    preserveRpWidgetSize(button);
    setMinimumSize(button->minimumSize());
}

ZaryaTextField::ZaryaTextField(
    const QString& placeholder,
    QWidget* parent,
    bool password)
    : QWidget(parent)
    , m_password(password)
{
    QWidget* field = password
        ? static_cast<QWidget*>(Ui::CreateChild<Ui::PasswordInput>(
            this,
            st::defaultInputField,
            rpl::single(placeholder)))
        : static_cast<QWidget*>(Ui::CreateChild<Ui::InputField>(
            this,
            st::defaultInputField,
            Ui::InputField::Mode::SingleLine,
            rpl::single(placeholder),
            TextWithTags()));
    m_field = field;
    field->show();
    setFocusProxy(field);
    setMinimumHeight(rpWidgetHeight(field));
    if (password) {
        auto* input = static_cast<Ui::PasswordInput*>(field);
        connect(input, &Ui::PasswordInput::changed, this, [this, input] {
            Q_EMIT textChanged(input->getLastText());
        });
    } else {
        auto* input = static_cast<Ui::InputField*>(field);
        input->changes() | rpl::on_next(
            [this, input] { Q_EMIT textChanged(input->getLastText()); },
            input->lifetime());
    }
}

QString ZaryaTextField::text() const
{
    return m_password
        ? static_cast<Ui::PasswordInput*>(m_field)->getLastText()
        : static_cast<Ui::InputField*>(m_field)->getLastText();
}

void ZaryaTextField::setText(const QString& text)
{
    if (m_password) {
        static_cast<Ui::PasswordInput*>(m_field)->setText(text);
    } else {
        static_cast<Ui::InputField*>(m_field)->setText(text);
    }
}

void ZaryaTextField::setPlaceholder(const QString& placeholder)
{
    if (m_password) {
        static_cast<Ui::PasswordInput*>(m_field)->setPlaceholder(rpl::single(placeholder));
    } else {
        static_cast<Ui::InputField*>(m_field)->setPlaceholder(rpl::single(placeholder));
    }
}

void ZaryaTextField::setReadOnly(bool readOnly)
{
    if (m_password) {
        static_cast<Ui::PasswordInput*>(m_field)->setReadOnly(readOnly);
    } else {
        static_cast<Ui::InputField*>(m_field)->rawTextEdit()->setReadOnly(readOnly);
    }
}

void ZaryaTextField::showError(bool show)
{
    if (show) {
        if (m_password) {
            static_cast<Ui::PasswordInput*>(m_field)->showErrorNoFocus();
        } else {
            static_cast<Ui::InputField*>(m_field)->showErrorNoFocus();
        }
    } else {
        if (m_password) {
            static_cast<Ui::PasswordInput*>(m_field)->hideError();
        } else {
            static_cast<Ui::InputField*>(m_field)->hideError();
        }
    }
}

void ZaryaTextField::setAccessibleLabel(const QString& label)
{
    m_field->setAccessibleName(label);
}

void ZaryaTextField::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    fitRpWidget(m_field, event->size().width());
}

ZaryaTextArea::ZaryaTextArea(
    const QString& placeholder,
    QWidget* parent,
    int minimumHeight)
    : QWidget(parent)
{
    auto* field = Ui::CreateChild<AccessibleTextAreaField>(
        this,
        st::defaultInputField,
        Ui::InputField::Mode::MultiLine,
        rpl::single(placeholder),
        TextWithTags());
    field->setMinHeight(minimumHeight);
    m_field = field;
    field->show();
    setFocusProxy(field);
    setMinimumHeight(minimumHeight);
    field->changes() | rpl::on_next(
        [this, field] { Q_EMIT textChanged(field->getLastText()); },
        field->lifetime());
}

QString ZaryaTextArea::text() const
{
    return static_cast<Ui::InputField*>(m_field)->getLastText();
}

void ZaryaTextArea::setText(const QString& text)
{
    static_cast<Ui::InputField*>(m_field)->setText(text);
}

void ZaryaTextArea::setReadOnly(bool readOnly)
{
    static_cast<AccessibleTextAreaField*>(m_field)->setReadOnly(readOnly);
}

void ZaryaTextArea::clear()
{
    setText(QString());
}

void ZaryaTextArea::setAccessibleLabel(const QString& label)
{
    m_field->setAccessibleName(label);
}

void ZaryaTextArea::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    auto* field = static_cast<Ui::InputField*>(m_field);
    field->resizeToWidth(std::max(event->size().width(), 0));
    field->resize(field->width(), std::max(event->size().height(), minimumHeight()));
}

ZaryaBodyText::ZaryaBodyText(const QString& text, QWidget* parent)
    : QWidget(parent)
{
    auto* label = Ui::CreateChild<Ui::FlatLabel>(this, text, st::defaultFlatLabel);
    label->setAccessibleName(text);
    m_label = label;
    setMinimumHeight(rpWidgetHeight(label));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label);
    label->show();
}

void ZaryaBodyText::setText(const QString& text)
{
    auto* label = static_cast<Ui::FlatLabel*>(m_label);
    label->setText(text);
    label->setAccessibleName(text);
    fitRpWidget(label, width());
    setMinimumHeight(rpWidgetHeight(label));
}

void ZaryaBodyText::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    fitRpWidget(m_label, event->size().width());
    setMinimumHeight(rpWidgetHeight(m_label));
}

ZaryaFormSection::ZaryaFormSection(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    auto* label = Ui::CreateChild<Ui::FlatLabel>(this, title, st::boxTitle);
    label->setAccessibleName(title);
    m_title = label;
    preserveRpWidgetSize(label);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(16, 16, 16, 16);
    m_layout->setSpacing(10);
    m_layout->addWidget(label);
    label->show();
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void ZaryaFormSection::addWidget(QWidget* widget)
{
    m_layout->addWidget(widget);
}

void ZaryaFormSection::addStretch()
{
    m_layout->addStretch();
}

ZaryaNumberField::ZaryaNumberField(
    const QString& placeholder,
    int minimum,
    int maximum,
    QWidget* parent)
    : QWidget(parent)
    , m_minimum(minimum)
    , m_maximum(maximum)
{
    const int digits = QString::number(std::max(std::abs(minimum), std::abs(maximum))).size();
    auto* field = Ui::CreateChild<Ui::NumberInput>(
        this,
        st::defaultInputField,
        rpl::single(placeholder),
        QString(),
        digits);
    m_field = field;
    field->show();
    setFocusProxy(field);
    setMinimumHeight(rpWidgetHeight(field));
    connect(field, &Ui::NumberInput::changed, this, [this] {
        Q_EMIT valueChanged(value());
    });
}

int ZaryaNumberField::value() const
{
    bool ok = false;
    const int parsed = static_cast<Ui::NumberInput*>(m_field)->getLastText().toInt(&ok);
    return std::clamp(ok ? parsed : m_minimum, m_minimum, m_maximum);
}

void ZaryaNumberField::setValue(int value)
{
    const int clamped = std::clamp(value, m_minimum, m_maximum);
    static_cast<Ui::NumberInput*>(m_field)->setText(
        (clamped == m_minimum && !m_specialValueText.isEmpty())
            ? QString()
            : QString::number(clamped));
}

void ZaryaNumberField::setSpecialValueText(const QString& text)
{
    const bool wasMinimum = value() == m_minimum;
    m_specialValueText = text;
    static_cast<Ui::NumberInput*>(m_field)->setPlaceholder(rpl::single(text));
    if (wasMinimum) {
        setValue(m_minimum);
    }
}

void ZaryaNumberField::showError(bool show)
{
    if (show) {
        static_cast<Ui::NumberInput*>(m_field)->showErrorNoFocus();
    } else {
        static_cast<Ui::NumberInput*>(m_field)->hideError();
    }
}

void ZaryaNumberField::setAccessibleLabel(const QString& label)
{
    m_field->setAccessibleName(label);
}

void ZaryaNumberField::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    fitRpWidget(m_field, event->size().width());
}

ZaryaCheckBox::ZaryaCheckBox(const QString& text, QWidget* parent, bool checked)
    : QWidget(parent)
{
    auto* checkbox = Ui::CreateChild<Ui::Checkbox>(this, text, checked);
    m_checkbox = checkbox;
    setFocusProxy(checkbox);
    preserveRpWidgetSize(checkbox);
    setMinimumHeight(checkbox->minimumHeight());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(checkbox);
    checkbox->show();
    layout->addStretch();
    checkbox->checkedChanges() | rpl::on_next(
        [this](bool value) { Q_EMIT toggled(value); },
        checkbox->lifetime());
}

bool ZaryaCheckBox::isChecked() const
{
    return static_cast<Ui::Checkbox*>(m_checkbox)->checked();
}

void ZaryaCheckBox::setChecked(bool checked)
{
    static_cast<Ui::Checkbox*>(m_checkbox)->setChecked(checked);
}

void ZaryaCheckBox::setText(const QString& text)
{
    auto* checkbox = static_cast<Ui::Checkbox*>(m_checkbox);
    checkbox->setText(text);
    preserveRpWidgetSize(checkbox);
    setMinimumHeight(checkbox->minimumHeight());
}

void ZaryaCheckBox::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    fitRpWidget(m_checkbox, event->size().width());
    setMinimumHeight(rpWidgetHeight(m_checkbox));
}

ZaryaRadioGroup::ZaryaRadioGroup(int value, QWidget* parent)
    : QWidget(parent)
    , m_group(std::make_shared<Ui::RadiobuttonGroup>(value))
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);
    m_group->setChangedCallback([this](int current) {
        Q_EMIT valueChanged(current);
    });
}

void ZaryaRadioGroup::addOption(int value, const QString& text)
{
    auto* button = Ui::CreateChild<Ui::Radiobutton>(this, m_group, value, text);
    if (m_options.isEmpty()) {
        setFocusProxy(button);
    }
    m_options.push_back({button, text});
    preserveRpWidgetSize(button);
    updateAccessibleNames();
    m_layout->addWidget(button);
    button->show();
}

int ZaryaRadioGroup::value() const
{
    return m_group->current();
}

void ZaryaRadioGroup::setValue(int value)
{
    m_group->setValue(value);
}

void ZaryaRadioGroup::setAccessibleLabel(const QString& label)
{
    m_accessibleLabel = label;
    updateAccessibleNames();
}

void ZaryaRadioGroup::updateAccessibleNames()
{
    for (const Option& option : m_options) {
        option.button->setAccessibleName(m_accessibleLabel.isEmpty()
                ? option.text
                : QStringLiteral("%1: %2").arg(m_accessibleLabel, option.text));
    }
}

ZaryaFormRow::ZaryaFormRow(const QString& label, QWidget* field, QWidget* parent)
    : QWidget(parent)
{
    auto* text = Ui::CreateChild<Ui::FlatLabel>(this, label, st::boxLabel);
    m_label = text;
    text->setAccessibleName(label);
    text->setFixedWidth(172);
    text->setMinimumHeight(rpWidgetHeight(text));
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(text, 0, Qt::AlignVCenter);
    text->show();
    layout->addWidget(field, 1);
    m_accessibleControl = dynamic_cast<ZaryaAccessibleFormControl*>(field);
    if (m_accessibleControl) {
        m_accessibleControl->setAccessibleLabel(label);
    }
    if (field->focusProxy() || field->focusPolicy() != Qt::NoFocus) {
        setFocusProxy(field);
    }
}

void ZaryaFormRow::setLabel(const QString& label)
{
    auto* text = static_cast<Ui::FlatLabel*>(m_label);
    text->setText(label);
    text->setAccessibleName(label);
    if (m_accessibleControl) {
        m_accessibleControl->setAccessibleLabel(label);
    }
}

ZaryaValidationMessage::ZaryaValidationMessage(QWidget* parent)
    : Ui::RpWidget(parent)
{
    auto* label = Ui::CreateChild<Ui::FlatLabel>(this, QString(), st::boxLabel);
    m_label = label;
    label->setMinimumHeight(rpWidgetHeight(label));
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label);
    label->show();
    connect(
        &ThemeManager::instance(),
        &ThemeManager::themeChanged,
        this,
        &ZaryaValidationMessage::updateColor);
    updateColor();
    hide();
}

void ZaryaValidationMessage::showMessage(
    const QString& message,
    QAccessible::AnnouncementPoliteness politeness)
{
    auto* label = static_cast<Ui::FlatLabel*>(m_label);
    label->setText(message);
    label->setAccessibleName(message);
    setAccessibleName(message);
    show();
    QAccessibleAnnouncementEvent event(this, message);
    event.setPoliteness(politeness);
    QAccessible::updateAccessibility(&event);
}

void ZaryaValidationMessage::clear()
{
    static_cast<Ui::FlatLabel*>(m_label)->setText(QString());
    setAccessibleName(QString());
    hide();
}

void ZaryaValidationMessage::updateColor()
{
    static_cast<Ui::FlatLabel*>(m_label)->setTextColorOverride(
        ThemeManager::instance().tokens().danger);
}

ZaryaDialogActionRow::ZaryaDialogActionRow(
    const QString& acceptText,
    const QString& cancelText,
    QWidget* parent)
    : ZaryaDialogActionRow(
        acceptText,
        cancelText,
        parent,
        ZaryaButtonRole::Primary)
{
}

ZaryaDialogActionRow::ZaryaDialogActionRow(
    const QString& acceptText,
    const QString& cancelText,
    QWidget* parent,
    ZaryaButtonRole acceptRole)
    : QWidget(parent)
{
    auto cancel = makeZaryaButton(this, cancelText, ZaryaButtonRole::Secondary);
    auto accept = makeZaryaButton(this, acceptText, acceptRole);
    preserveRpWidgetSize(cancel.data());
    preserveRpWidgetSize(accept.data());
    const int buttonHeight = std::max(
        cancel->minimumHeight(),
        accept->minimumHeight());
    m_accept = accept.data();
    static_cast<Ui::RoundButton*>(cancel.data())->setClickedCallback([this] {
        QMetaObject::invokeMethod(this, [this] { Q_EMIT rejected(); }, Qt::QueuedConnection);
    });
    static_cast<Ui::RoundButton*>(accept.data())->setClickedCallback([this] {
        QMetaObject::invokeMethod(this, [this] { Q_EMIT accepted(); }, Qt::QueuedConnection);
    });

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addStretch();
    cancel->show();
    accept->show();
    layout->addWidget(cancel.release());
    layout->addWidget(accept.release());
    setMinimumHeight(buttonHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void ZaryaDialogActionRow::focusAccept()
{
    m_accept->setFocus(Qt::OtherFocusReason);
}

} // namespace zarya
