#pragma once

#include <QString>
#include <QWidget>
#include <memory>

class QResizeEvent;
class QVBoxLayout;

namespace Ui {
class RadiobuttonGroup;
}

namespace zarya {

enum class ZaryaButtonRole;

class ZaryaActionButton final : public QWidget {
    Q_OBJECT

public:
    ZaryaActionButton(const QString& text, QWidget* parent);
    ZaryaActionButton(const QString& text, QWidget* parent, ZaryaButtonRole role);
    void setText(const QString& text);

Q_SIGNALS:
    void clicked();

private:
    QWidget* m_button = nullptr; // Ui::RoundButton*
};

class ZaryaTextField final : public QWidget {
    Q_OBJECT

public:
    explicit ZaryaTextField(
        const QString& placeholder,
        QWidget* parent = nullptr,
        bool password = false);

    [[nodiscard]] QString text() const;
    void setText(const QString& text);
    void setPlaceholder(const QString& placeholder);
    void setReadOnly(bool readOnly);
    void showError(bool show = true);

Q_SIGNALS:
    void textChanged(const QString& text);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    bool m_password = false;
    QWidget* m_field = nullptr; // Ui::InputField*
};

class ZaryaTextArea final : public QWidget {
    Q_OBJECT

public:
    explicit ZaryaTextArea(
        const QString& placeholder,
        QWidget* parent = nullptr,
        int minimumHeight = 140);

    [[nodiscard]] QString text() const;
    void setText(const QString& text);
    void setReadOnly(bool readOnly);
    void clear();

Q_SIGNALS:
    void textChanged(const QString& text);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QWidget* m_field = nullptr; // Ui::InputField*
};

class ZaryaBodyText final : public QWidget {
    Q_OBJECT

public:
    explicit ZaryaBodyText(const QString& text = {}, QWidget* parent = nullptr);

    void setText(const QString& text);

private:
    QWidget* m_label = nullptr; // Ui::FlatLabel*
};

class ZaryaFormSection final : public QWidget {
    Q_OBJECT

public:
    explicit ZaryaFormSection(const QString& title, QWidget* parent = nullptr);

    void addWidget(QWidget* widget);
    void addStretch();

private:
    QWidget* m_title = nullptr; // Ui::FlatLabel*
    QVBoxLayout* m_layout = nullptr;
};

class ZaryaNumberField final : public QWidget {
    Q_OBJECT

public:
    ZaryaNumberField(
        const QString& placeholder,
        int minimum,
        int maximum,
        QWidget* parent = nullptr);

    [[nodiscard]] int value() const;
    void setValue(int value);
    void setSpecialValueText(const QString& text);
    void showError(bool show = true);

Q_SIGNALS:
    void valueChanged(int value);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    int m_minimum = 0;
    int m_maximum = 0;
    QString m_specialValueText;
    QWidget* m_field = nullptr; // Ui::NumberInput*
};

class ZaryaCheckBox final : public QWidget {
    Q_OBJECT

public:
    explicit ZaryaCheckBox(
        const QString& text,
        QWidget* parent = nullptr,
        bool checked = false);

    [[nodiscard]] bool isChecked() const;
    void setChecked(bool checked);
    void setText(const QString& text);

Q_SIGNALS:
    void toggled(bool checked);

private:
    QWidget* m_checkbox = nullptr; // Ui::Checkbox*
};

class ZaryaRadioGroup final : public QWidget {
    Q_OBJECT

public:
    explicit ZaryaRadioGroup(int value, QWidget* parent = nullptr);

    void addOption(int value, const QString& text);
    [[nodiscard]] int value() const;
    void setValue(int value);

Q_SIGNALS:
    void valueChanged(int value);

private:
    std::shared_ptr<Ui::RadiobuttonGroup> m_group;
    QVBoxLayout* m_layout = nullptr;
};

class ZaryaFormRow final : public QWidget {
    Q_OBJECT

public:
    ZaryaFormRow(
        const QString& label,
        QWidget* field,
        QWidget* parent = nullptr);

    void setLabel(const QString& label);

private:
    QWidget* m_label = nullptr; // Ui::FlatLabel*
};

class ZaryaValidationMessage final : public QWidget {
    Q_OBJECT

public:
    explicit ZaryaValidationMessage(QWidget* parent = nullptr);

    void showMessage(const QString& message);
    void clear();

private:
    void updateColor();

    QWidget* m_label = nullptr; // Ui::FlatLabel*
};

class ZaryaDialogActionRow final : public QWidget {
    Q_OBJECT

public:
    ZaryaDialogActionRow(
        const QString& acceptText,
        const QString& cancelText,
        QWidget* parent);
    ZaryaDialogActionRow(
        const QString& acceptText,
        const QString& cancelText,
        QWidget* parent,
        ZaryaButtonRole acceptRole);

    void focusAccept();

Q_SIGNALS:
    void accepted();
    void rejected();

private:
    QWidget* m_accept = nullptr; // Ui::RoundButton*
};

} // namespace zarya
