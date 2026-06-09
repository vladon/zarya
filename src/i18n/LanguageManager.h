#pragma once

#include "i18n/LanguageInfo.h"

#include <QObject>
#include <QString>
#include <QVector>

class QTranslator;

namespace zarya {

class LanguageManager : public QObject {
    Q_OBJECT

public:
    static LanguageManager& instance();

    QVector<LanguageInfo> availableLanguages() const;

    QString currentLanguageCode() const;
    bool setLanguage(const QString& languageCode, QString* errorMessage = nullptr);

    QString systemLanguageCode() const;
    QString effectiveLanguageCode() const;

    bool installTranslators(QString* errorMessage = nullptr);
    void removeTranslators();

signals:
    void languageChanged(const QString& languageCode);

private:
    explicit LanguageManager(QObject* parent = nullptr);

    QString resolveLanguageCode(const QString& configuredCode) const;
    QString translationFilePath(const QString& languageCode) const;

    QVector<QTranslator*> m_translators;
};

} // namespace zarya
