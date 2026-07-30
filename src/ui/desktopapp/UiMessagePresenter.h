#pragma once

#include <QCoreApplication>
#include <QString>

class QWidget;

namespace zarya {

enum class UiMessageTone {
    Information,
    Warning,
    Error,
};

class UiMessagePresenter {
    Q_DECLARE_TR_FUNCTIONS(UiMessagePresenter)

public:
    static void information(QWidget* parent, const QString& title, const QString& text);
    static void warning(QWidget* parent, const QString& title, const QString& text);
    static void error(QWidget* parent, const QString& title, const QString& text);

    [[nodiscard]] static bool confirm(
        QWidget* parent,
        const QString& title,
        const QString& text,
        const QString& acceptText = {},
        bool destructive = false);

private:
    static void show(
        QWidget* parent,
        const QString& title,
        const QString& text,
        UiMessageTone tone);
};

} // namespace zarya
