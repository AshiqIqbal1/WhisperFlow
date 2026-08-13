#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMap>

class GlobalHotkey;
class ModelManager;
class QKeySequenceEdit;
class QLabel;
class QProgressBar;
class QPushButton;
class QRadioButton;

// Settings — model management + the global shortcut.
// One row per catalog model:
//   [radio: active] Name        size      [Download|Cancel|Delete] [progress]
// The radio is only enabled once that model's file is on disk.
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(ModelManager *models, GlobalHotkey *hotkey,
                            QWidget *parent = nullptr);

private slots:
    void onDownloadClicked(const QString &id);
    void onDeleteClicked(const QString &id);
    void onProgress(const QString &id, qint64 received, qint64 total);
    void onFinished(const QString &id, bool ok, const QString &error);
    void onHotkeyEdited();

private:
    struct Row
    {
        QRadioButton *active = nullptr;
        QLabel *size = nullptr;
        QPushButton *action = nullptr;
        QProgressBar *progress = nullptr;
    };

    void refreshRow(const QString &id);

    ModelManager *m_models = nullptr;
    GlobalHotkey *m_hotkey = nullptr;
    QKeySequenceEdit *m_hotkeyEdit = nullptr;
    QLabel *m_hotkeyStatus = nullptr;
    QMap<QString, Row> m_rows;
};

#endif // SETTINGSDIALOG_H
