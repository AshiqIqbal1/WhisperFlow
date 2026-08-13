#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "transcriptcard.h"

#include <QList>
#include <QMainWindow>

class QDragEnterEvent;
class QDropEvent;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QTimer;
class QVBoxLayout;
class QWidget;
class RecordButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void toggleRecording();
    void applyFilter(const QString &needle);

private:
    QWidget *buildHeader();
    QWidget *buildList();
    QWidget *buildFooter();

    void addTranscript(const Transcript &t, bool atTop = true);
    void removeTranscript(const QString &id);
    void refreshEmptyState();
    void loadSampleData();
    void flashStatus(const QString &message);

    QLineEdit    *m_search = nullptr;
    QVBoxLayout  *m_listLayout = nullptr;
    QWidget      *m_emptyState = nullptr;
    RecordButton *m_record = nullptr;
    QLabel       *m_status = nullptr;
    QTimer       *m_levelTimer = nullptr;
    QTimer       *m_statusTimer = nullptr;

    QList<TranscriptCard *> m_cards;
    int m_nextId = 1;
    int m_levelTick = 0;
};

#endif // MAINWINDOW_H
