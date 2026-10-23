#include "Language.h"

#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

namespace i18n {

QList<Language> available()
{
    return {
        {QString(), QCoreApplication::translate("Language", "System Default")},
        {"en", "English"},
        {"zh_CN", QString::fromUtf8("简体中文")},
        {"ja", QString::fromUtf8("日本語")},
    };
}

QString configured()
{
    return QSettings().value("ui/language").toString();
}

void setConfigured(const QString &code)
{
    QSettings().setValue("ui/language", code);
}

QString effective()
{
    const QString code = configured();
    if (!code.isEmpty())
        return code;
    for (const QString &ui : QLocale::system().uiLanguages()) {
        if (ui.startsWith("zh", Qt::CaseInsensitive))
            return "zh_CN";
        if (ui.startsWith("ja", Qt::CaseInsensitive))
            return "ja";
        if (ui.startsWith("en", Qt::CaseInsensitive))
            return "en";
    }
    return "en";
}

void install(QApplication &app)
{
    const QString lang = effective();
    if (lang == "en")
        return;   // source strings are English
    // Both translators live as long as the application.
    auto *own = new QTranslator(&app);
    if (own->load(":/i18n/winroute_" + lang))
        QApplication::installTranslator(own);
    auto *qt = new QTranslator(&app);
    if (qt->load(":/i18n/qtbase_" + lang))
        QApplication::installTranslator(qt);
    QLocale::setDefault(QLocale(lang));
}

} // namespace i18n
