#include "mainwindow.h"

#include "icons.h"
#include "recordbutton.h"
#include "theme.h"

#include <QApplication>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QScrollArea>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <cmath>

namespace {
constexpr int kWindowW = 560;
constexpr int kWindowH = 760;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("WhisperFlow"));
    resize(kWindowW, kWindowH);
    setAcceptDrops(true);
    setStyleSheet(Theme::styleSheet());

    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("root"));
    auto *layout = new QVBoxLayout(root);
    layout->setContentsMargins(20, 18, 20, 16);
    layout->setSpacing(14);

    layout->addWidget(buildHeader());
    layout->addWidget(buildList(), /*stretch=*/1);
    layout->addWidget(buildFooter());

    setCentralWidget(root);

    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("statusLabel"));
    statusBar()->addWidget(m_status);
    statusBar()->setStyleSheet(QStringLiteral("QStatusBar{background:#141416;border-top:1px solid #26262A;}"));

    // Fake mic-level animation so the record button has something to react
    // to. Replace with real levels from your audio input once wired up.
    m_levelTimer = new QTimer(this);
    connect(m_levelTimer, &QTimer::timeout, this, [this] {
        m_levelTick++;
        const qreal level = 0.35 + 0.35 * std::abs(std::sin(m_levelTick * 0.35));
        m_record->setLevel(level);
    });

    m_statusTimer = new QTimer(this);
    m_statusTimer->setSingleShot(true);
    connect(m_statusTimer, &QTimer::timeout, this, [this] { m_status->clear(); });

    loadSampleData();
    refreshEmptyState();
}

MainWindow::~MainWindow() = default;

QWidget *MainWindow::buildHeader()
{
    auto *wrap = new QWidget(this);
    auto *row = new QHBoxLayout(wrap);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(10);

    auto *searchIcon = new QLabel(wrap);
    searchIcon->setPixmap(Icons::icon(Icons::Search, Theme::TextFaint, 15).pixmap(15, 15));

    m_search = new QLineEdit(wrap);
    m_search->setObjectName(QStringLiteral("searchBar"));
    m_search->setPlaceholderText(tr("Search in transcriptions"));
    m_search->setClearButtonEnabled(true);
    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::applyFilter);

    // Icon lives visually inside the field via a leading-margin trick.
    m_search->setTextMargins(24, 0, 0, 0);
    searchIcon->setParent(m_search);
    searchIcon->move(12, 11);

    row->addWidget(m_search, 1);
    return wrap;
}

QWidget *MainWindow::buildList()
{
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *body = new QWidget(scroll);
    body->setObjectName(QStringLiteral("scrollBody"));
    m_listLayout = new QVBoxLayout(body);
    m_listLayout->setContentsMargins(2, 2, 2, 2);
    m_listLayout->setSpacing(12);
    m_listLayout->addStretch(1); // keeps cards top-aligned as list grows/shrinks

    m_emptyState = new QWidget(body);
    auto *emptyLay = new QVBoxLayout(m_emptyState);
    emptyLay->setContentsMargins(0, 60, 0, 0);
    auto *emptyLabel = new QLabel(tr("No transcriptions yet\nPress record to get started"), m_emptyState);
    emptyLabel->setObjectName(QStringLiteral("emptyTitle"));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLay->addWidget(emptyLabel);
    m_listLayout->insertWidget(0, m_emptyState);

    scroll->setWidget(body);
    return scroll;
}

QWidget *MainWindow::buildFooter()
{
    auto *wrap = new QWidget(this);
    auto *outer = new QVBoxLayout(wrap);
    outer->setContentsMargins(0, 4, 0, 0);
    outer->setSpacing(10);

    m_record = new RecordButton(wrap);
    connect(m_record, &QAbstractButton::clicked, this, &MainWindow::toggleRecording);

    auto *centerRow = new QHBoxLayout;
    centerRow->addStretch(1);
    centerRow->addWidget(m_record);
    centerRow->addStretch(1);
    outer->addLayout(centerRow);

    auto *bottomRow = new QHBoxLayout;
    bottomRow->setSpacing(8);

    auto *hint = new QLabel(wrap);
    hint->setObjectName(QStringLiteral("hint"));
    hint->setText(tr("Drop audio file here to transcribe"));
    bottomRow->addWidget(hint);
    bottomRow->addStretch(1);

    auto *mic = new QToolButton(wrap);
    mic->setObjectName(QStringLiteral("footerBtn"));
    mic->setIcon(Icons::icon(Icons::Mic, Theme::TextMuted, 16));
    mic->setToolTip(tr("Input device"));
    bottomRow->addWidget(mic);

    auto *trash = new QToolButton(wrap);
    trash->setObjectName(QStringLiteral("footerBtn"));
    trash->setIcon(Icons::icon(Icons::Trash, Theme::TextMuted, 16));
    trash->setToolTip(tr("Clear all"));
    connect(trash, &QToolButton::clicked, this, [this] {
        for (auto *card : std::as_const(m_cards))
            card->deleteLater();
        m_cards.clear();
        refreshEmptyState();
        flashStatus(tr("Cleared"));
    });
    bottomRow->addWidget(trash);

    auto *settings = new QToolButton(wrap);
    settings->setObjectName(QStringLiteral("footerBtn"));
    settings->setIcon(Icons::icon(Icons::Settings, Theme::TextMuted, 16));
    settings->setToolTip(tr("Settings"));
    bottomRow->addWidget(settings);

    outer->addLayout(bottomRow);
    return wrap;
}

void MainWindow::toggleRecording()
{
    const bool nowRecording = !m_record->isRecording();
    m_record->setRecording(nowRecording);

    if (nowRecording) {
        m_levelTick = 0;
        m_levelTimer->start(90);
        flashStatus(tr("Recording…"));
    } else {
        m_levelTimer->stop();
        flashStatus(tr("Transcribing…"));

        // Placeholder: real pipeline should call whisper.cpp here and then
        // call addTranscript() with the actual result.
        QTimer::singleShot(900, this, [this] {
            addTranscript({QString::number(m_nextId++),
                           tr("New recording — replace this with the real "
                              "transcription once whisper.cpp is wired in."),
                           QDateTime::currentDateTime(), 0});
            flashStatus(tr("Done"));
        });
    }
}

void MainWindow::addTranscript(const Transcript &t, bool atTop)
{
    auto *card = new TranscriptCard(t, this);
    connect(card, &TranscriptCard::deleteRequested, this, &MainWindow::removeTranscript);
    connect(card, &TranscriptCard::copyRequested, this, [this](const QString &id) {
        for (auto *c : std::as_const(m_cards)) {
            if (c->data().id == id) {
                QGuiApplication::clipboard()->setText(c->data().text);
                flashStatus(tr("Copied to clipboard"));
                break;
            }
        }
    });
    connect(card, &TranscriptCard::retryRequested, this, [this](const QString &id) {
        flashStatus(tr("Re-transcribing %1…").arg(id));
    });
    connect(card, &TranscriptCard::playRequested, this, [this](const QString &id) {
        flashStatus(tr("Playing %1…").arg(id));
    });

    const int insertPos = atTop ? 1 : m_listLayout->count() - 1; // slot 0 is the empty state
    m_listLayout->insertWidget(insertPos, card);
    atTop ? m_cards.prepend(card) : m_cards.append(card);

    refreshEmptyState();
}

void MainWindow::removeTranscript(const QString &id)
{
    for (int i = 0; i < m_cards.size(); ++i) {
        if (m_cards[i]->data().id == id) {
            m_cards[i]->deleteLater();
            m_cards.removeAt(i);
            break;
        }
    }
    refreshEmptyState();
    flashStatus(tr("Deleted"));
}

void MainWindow::applyFilter(const QString &needle)
{
    bool anyVisible = false;
    for (auto *card : std::as_const(m_cards)) {
        const bool match = card->matches(needle);
        card->setVisible(match);
        anyVisible |= match;
    }
    m_emptyState->setVisible(!anyVisible);
}

void MainWindow::refreshEmptyState()
{
    m_emptyState->setVisible(m_cards.isEmpty());
}

void MainWindow::flashStatus(const QString &message)
{
    m_status->setText(message);
    m_statusTimer->start(2500);
}

void MainWindow::loadSampleData()
{
    addTranscript({QStringLiteral("s3"),
                   tr("I think I'll give you a ramble on what's going on and how I may want "
                      "the thesis to go around. Then we can discuss it together. But the main "
                      "thesis presentations, I mean, not the presentation, the actual written "
                      "thesis needs a clear structure before I record anything else."),
                   QDateTime(QDate(2026, 8, 9), QTime(15, 10)), 187},
                  false);
    addTranscript({QStringLiteral("s2"),
                   tr("I don't really like this. This is already saying the representation "
                      "needs to be aligned. That's the main thesis statement. That's not what "
                      "we want. I think that should be more like a finding analysis instead."),
                   QDateTime(QDate(2026, 8, 9), QTime(15, 3)), 94},
                  false);
    addTranscript({QStringLiteral("s1"),
                   tr("Quick note to self: check the CMake install rules before the Windows "
                      "build, and remember to bundle the whisper model file with windeployqt."),
                   QDateTime(QDate(2026, 8, 9), QTime(14, 38)), 41},
                  false);
    m_nextId = 4;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !m_search->hasFocus()) {
        toggleRecording();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;

    flashStatus(tr("Transcribing dropped file…"));
    const QString fileName = urls.first().fileName();
    QTimer::singleShot(900, this, [this, fileName] {
        addTranscript({QString::number(m_nextId++),
                       tr("Transcription of %1 — replace with real whisper.cpp output.")
                           .arg(fileName),
                       QDateTime::currentDateTime(), 0});
        flashStatus(tr("Done"));
    });
}
