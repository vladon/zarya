#include "i18n/LanguageManager.h"

#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

namespace zarya {

namespace {

constexpr auto kSystemLanguage = "system";

} // namespace

LanguageManager& LanguageManager::instance()
{
    static LanguageManager manager;
    return manager;
}

LanguageManager::LanguageManager(QObject* parent)
    : QObject(parent)
{
}

QVector<LanguageInfo> LanguageManager::availableLanguages() const
{
  return {
      {QStringLiteral("system"), tr("System default"), tr("System default"), true},
      {QStringLiteral("en"), QStringLiteral("English"), QStringLiteral("English"), true},
      {QStringLiteral("ru"), QStringLiteral("Русский"), QStringLiteral("Russian"), true},
  };
}

QString LanguageManager::currentLanguageCode() const
{
    return AppSettings::instance().languageCode();
}

QString LanguageManager::systemLanguageCode() const
{
    const QString locale = QLocale::system().name().toLower();
    if (locale.startsWith(QStringLiteral("ru"))) {
        return QStringLiteral("ru");
    }
    return QStringLiteral("en");
}

QString LanguageManager::resolveLanguageCode(const QString& configuredCode) const
{
    const QString normalized = configuredCode.trimmed().toLower();
    if (normalized.isEmpty() || normalized == QLatin1String(kSystemLanguage)) {
        return systemLanguageCode();
    }
    if (normalized == QStringLiteral("ru")) {
        return QStringLiteral("ru");
    }
    return QStringLiteral("en");
}

QString LanguageManager::effectiveLanguageCode() const
{
    return resolveLanguageCode(currentLanguageCode());
}

QString LanguageManager::translationFilePath(const QString& languageCode) const
{
    const QString fileName = QStringLiteral("zarya_%1.qm").arg(languageCode);
    const QStringList candidates = {
        AppPaths::translationsDir() + QLatin1Char('/') + fileName,
        AppPaths::applicationDir() + QStringLiteral("/translations/") + fileName,
        AppPaths::applicationDir() + QStringLiteral("/../share/zarya/translations/") + fileName,
    };
    for (const QString& path : candidates) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return candidates.first();
}

bool LanguageManager::installTranslators(QString* errorMessage)
{
    removeTranslators();

    const QString language = effectiveLanguageCode();
    const QString path = translationFilePath(language);
    if (!QFile::exists(path)) {
        if (language != QStringLiteral("en")) {
            if (errorMessage) {
                *errorMessage = tr("Translation file not found: %1").arg(path);
            }
        }
        return language == QStringLiteral("en");
    }

    auto* translator = new QTranslator(this);
    if (!translator->load(path)) {
        if (errorMessage) {
            *errorMessage = tr("Failed to load translation: %1").arg(path);
        }
        delete translator;
        return false;
    }
    QCoreApplication::installTranslator(translator);
    m_translators.append(translator);
    return true;
}

void LanguageManager::removeTranslators()
{
    for (QTranslator* translator : m_translators) {
        QCoreApplication::removeTranslator(translator);
        translator->deleteLater();
    }
    m_translators.clear();
}

bool LanguageManager::setLanguage(const QString& languageCode, QString* errorMessage)
{
    const QString normalized = languageCode.trimmed().toLower();
    if (normalized != QLatin1String(kSystemLanguage) && normalized != QStringLiteral("en")
        && normalized != QStringLiteral("ru")) {
        if (errorMessage) {
            *errorMessage = tr("Unsupported language: %1").arg(languageCode);
        }
        return false;
    }

    AppSettings::instance().setLanguageCode(normalized.isEmpty() ? QLatin1String(kSystemLanguage)
                                                                  : normalized);
    installTranslators(errorMessage);
    emit languageChanged(normalized);
    return true;
}

} // namespace zarya
