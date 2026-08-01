#pragma once

#include <QCoreApplication>
#include <QString>
#include <QVector>

class QWidget;

namespace zarya {

enum class UiMessageTone {
    Information,
    Warning,
    Error,
};

enum class UiMessageActionRole {
    Primary,
    Secondary,
    Destructive,
};

struct UiMessageAction {
    QString id;
    QString text;
    UiMessageActionRole role = UiMessageActionRole::Secondary;
    bool isDefault = false;
    bool isCancel = false;
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

    [[nodiscard]] static QString choose(
        QWidget* parent,
        const QString& title,
        const QString& text,
        UiMessageTone tone,
        const QVector<UiMessageAction>& actions);

private:
    static void show(
        QWidget* parent,
        const QString& title,
        const QString& text,
        UiMessageTone tone);
};

} // namespace zarya
