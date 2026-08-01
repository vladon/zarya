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

#include <QHBoxLayout>
#include <QMetaObject>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <algorithm>
#include <cstdlib>
#include <rpl/rpl.h>

namespace zarya {
namespace {

void fitField(QWidget* field, int width)
{
    field->resize(std::max(width, 0), field->sizeHint().height());
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
    static_cast<Ui::RoundButton*>(m_button)->setClickedCallback([this] {
        QMetaObject::invokeMethod(this, [this] { Q_EMIT clicked(); }, Qt::QueuedConnection);
    });
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(button.release());
}

void ZaryaActionButton::setText(const QString& text)
{
    auto* button = static_cast<Ui::RoundButton*>(m_button);
    button->setText(rpl::single(text));
    button->setAccessibleName(text);
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
    setFocusProxy(field);
    setMinimumHeight(field->sizeHint().height());
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

void ZaryaTextField::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    fitField(m_field, event->size().width());
}

ZaryaTextArea::ZaryaTextArea(
    const QString& placeholder,
    QWidget* parent,
    int minimumHeight)
    : QWidget(parent)
{
    auto* field = Ui::CreateChild<Ui::InputField>(
        this,
        st::defaultInputField,
        Ui::InputField::Mode::MultiLine,
        rpl::single(placeholder),
        TextWithTags());
    field->setMinHeight(minimumHeight);
    m_field = field;
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

void ZaryaTextArea::clear()
{
    setText(QString());
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
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label);
}

void ZaryaBodyText::setText(const QString& text)
{
    auto* label = static_cast<Ui::FlatLabel*>(m_label);
    label->setText(text);
    label->setAccessibleName(text);
}

ZaryaFormSection::ZaryaFormSection(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    auto* label = Ui::CreateChild<Ui::FlatLabel>(this, title, st::boxTitle);
    label->setAccessibleName(title);
    m_title = label;

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(16, 16, 16, 16);
    m_layout->setSpacing(10);
    m_layout->addWidget(label);
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
    setFocusProxy(field);
    setMinimumHeight(field->sizeHint().height());
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

void ZaryaNumberField::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    fitField(m_field, event->size().width());
}

ZaryaCheckBox::ZaryaCheckBox(const QString& text, QWidget* parent, bool checked)
    : QWidget(parent)
{
    auto* checkbox = Ui::CreateChild<Ui::Checkbox>(this, text, checked);
    m_checkbox = checkbox;
    setFocusProxy(checkbox);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(checkbox);
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
    static_cast<Ui::Checkbox*>(m_checkbox)->setText(text);
}

ZaryaFormRow::ZaryaFormRow(const QString& label, QWidget* field, QWidget* parent)
    : QWidget(parent)
{
    auto* text = Ui::CreateChild<Ui::FlatLabel>(this, label, st::boxLabel);
    m_label = text;
    text->setAccessibleName(label);
    text->setFixedWidth(172);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addWidget(text, 0, Qt::AlignVCenter);
    layout->addWidget(field, 1);
    setFocusProxy(field);
}

void ZaryaFormRow::setLabel(const QString& label)
{
    auto* text = static_cast<Ui::FlatLabel*>(m_label);
    text->setText(label);
    text->setAccessibleName(label);
}

ZaryaValidationMessage::ZaryaValidationMessage(QWidget* parent)
    : QWidget(parent)
{
    auto* label = Ui::CreateChild<Ui::FlatLabel>(this, QString(), st::boxLabel);
    m_label = label;
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label);
    connect(
        &ThemeManager::instance(),
        &ThemeManager::themeChanged,
        this,
        &ZaryaValidationMessage::updateColor);
    updateColor();
    hide();
}

void ZaryaValidationMessage::showMessage(const QString& message)
{
    auto* label = static_cast<Ui::FlatLabel*>(m_label);
    label->setText(message);
    label->setAccessibleName(message);
    show();
}

void ZaryaValidationMessage::clear()
{
    static_cast<Ui::FlatLabel*>(m_label)->setText(QString());
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
    layout->addWidget(cancel.release());
    layout->addWidget(accept.release());
}

void ZaryaDialogActionRow::focusAccept()
{
    m_accept->setFocus(Qt::OtherFocusReason);
}

} // namespace zarya
