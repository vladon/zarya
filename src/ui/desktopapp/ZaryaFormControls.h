#pragma once

#include <QString>
#include <QWidget>

class QResizeEvent;

namespace zarya {

enum class ZaryaButtonRole;

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
    void showError(bool show = true);

Q_SIGNALS:
    void textChanged(const QString& text);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    bool m_password = false;
    QWidget* m_field = nullptr; // Ui::InputField*
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
