#pragma once

#include <QList>
#include <QString>

class QApplication;

// UI language selection. The choice is stored in the settings and applied at
// start-up; changing it restarts the application.
namespace i18n {

struct Language {
    QString code;         // "" = follow the system, otherwise "en", "zh_CN", "ja"
    QString nativeName;   // shown in the menu in its own language
};

QList<Language> available();
QString configured();
void setConfigured(const QString &code);
QString effective();   // configured language, or the best match for the system UI language
void install(QApplication &app);

} // namespace i18n
