#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Give QSettings and QStandardPaths::AppLocalDataLocation a stable home
    // (~/Library/Application Support/WhisperFlow on macOS, %LOCALAPPDATA% on
    // Windows). Must happen before anything touches settings or the model dir.
    QCoreApplication::setOrganizationName(QStringLiteral("WhisperFlow"));
    QCoreApplication::setApplicationName(QStringLiteral("WhisperFlow"));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "WhisperFlow_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return QApplication::exec();
}
