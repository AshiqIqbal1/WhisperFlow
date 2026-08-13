#include "mainwindow.h"

#include "audioclipstore.h"
#include "audiofiledecoder.h"
#include "audiorecorder.h"
#include "globalhotkey.h"
#include "icons.h"
#include "modelcatalog.h"
#include "modelmanager.h"
#include "recordbutton.h"
#include "settingsdialog.h"
#include "theme.h"
#include "transcriptstore.h"
#include "whisperengine.h"

#include <QApplication>
#include <QAudioOutput>
#include <QClipboard>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QMimeData>
#include <QScrollArea>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace {
constexpr int kWindowW = 560;
constexpr int kWindowH = 760;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_engine(std::make_unique<WhisperEngine>())
{
    setWindowTitle(tr("WhisperFlow"));
    resize(kWindowW, kWindowH);
    setAcceptDrops(true);
    setStyleSheet(Theme::styleSheet());

    m_models = new ModelManager(this);
    m_recorder = new AudioRecorder(this);
    connect(m_recorder, &AudioRecorder::levelChanged, this,
            [this](qreal level) { m_record->setLevel(level); });

    m_player = new QMediaPlayer(this);
    m_audioOut = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOut);

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

    m_statusTimer = new QTimer(this);
    m_statusTimer->setSingleShot(true);
    connect(m_statusTimer, &QTimer::timeout, this, [this] { m_status->clear(); });

    connect(&m_transcribeWatcher, &QFutureWatcher<QString>::finished, this, [this] {
        m_transcribing = false;
        const QString text = m_transcribeWatcher.result();
        if (text.isEmpty()) {
            flashStatus(tr("Transcription failed: %1").arg(m_engine->lastError()));
            return;
        }

        const auto props = m_transcribeWatcher.property("job").toMap();
        const QString clipId = props.value(QStringLiteral("clipId")).toString();
        const int durationSec = props.value(QStringLiteral("durationSec")).toInt();
        const bool isRetry = props.value(QStringLiteral("isRetry")).toBool();

        if (isRetry) {
            // Update the existing card in place: delete + re-add keeps it simple.
            for (auto *card : std::as_const(m_cards)) {
                if (card->data().id == clipId) {
                    Transcript updated = card->data();
                    updated.text = text;
                    m_cards.removeOne(card);
                    card->deleteLater();
                    addCard(updated, true);
                    break;
                }
            }
        } else {
            addCard({clipId, text, QDateTime::currentDateTime(), durationSec}, true);
        }
        persist();
        flashStatus(tr("Done"));
    });

    // Global hotkey — works even when another app has focus.
    m_hotkey = new GlobalHotkey(this);
    connect(m_hotkey, &GlobalHotkey::activated, this, [this] {
        show();
        raise();
        activateWindow();
        toggleRecording();
    });
    if (!m_hotkey->registerHotkey())
        flashStatus(tr("Global hotkey %1 unavailable (in use by another app)")
                        .arg(m_hotkey->comboLabel()));

    // Restore previous sessions' transcripts.
    const QList<Transcript> saved = TranscriptStore::load();
    for (const Transcript &t : saved)
        addCard(t, false);

    refreshEmptyState();
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_transcribing) {
        // Let the worker finish writing before the engine is torn down.
        m_transcribeWatcher.waitForFinished();
    }
    persist();
    QMainWindow::closeEvent(event);
}

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
    hint->setText(tr("Drop audio file here to transcribe  ·  %1 to record")
                      .arg(m_hotkey ? m_hotkey->comboLabel() : QStringLiteral("…")));
    bottomRow->addWidget(hint);
    bottomRow->addStretch(1);

    auto *trash = new QToolButton(wrap);
    trash->setObjectName(QStringLiteral("footerBtn"));
    trash->setIcon(Icons::icon(Icons::Trash, Theme::TextMuted, 16));
    trash->setToolTip(tr("Clear all"));
    connect(trash, &QToolButton::clicked, this, [this] {
        for (auto *card : std::as_const(m_cards)) {
            AudioClipStore::remove(card->data().id);
            card->deleteLater();
        }
        m_cards.clear();
        persist();
        refreshEmptyState();
        flashStatus(tr("Cleared"));
    });
    bottomRow->addWidget(trash);

    auto *settings = new QToolButton(wrap);
    settings->setObjectName(QStringLiteral("footerBtn"));
    settings->setIcon(Icons::icon(Icons::Settings, Theme::TextMuted, 16));
    settings->setToolTip(tr("Settings"));
    connect(settings, &QToolButton::clicked, this, &MainWindow::openSettings);
    bottomRow->addWidget(settings);

    outer->addLayout(bottomRow);
    return wrap;
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_models, this);
    dialog.exec();
}

bool MainWindow::ensureModelReady()
{
    const QString id = m_models->activeModelId();
    if (m_models->isDownloaded(id))
        return true;

    flashStatus(tr("Model \"%1\" is not downloaded yet — opening Settings").arg(id));
    openSettings();
    return m_models->isDownloaded(m_models->activeModelId());
}

void MainWindow::toggleRecording()
{
    if (m_transcribing) {
        flashStatus(tr("Still transcribing the previous recording…"));
        return;
    }

    if (!m_recorder->isRecording()) {
        if (!ensureModelReady())
            return;
        if (!m_recorder->start()) {
            flashStatus(m_recorder->lastError());
            return;
        }
        m_recordClock.start();
        m_record->setRecording(true);
        flashStatus(tr("Recording…"));
    } else {
        std::vector<float> samples = m_recorder->stop();
        m_record->setRecording(false);

        const int durationSec = int(m_recordClock.elapsed() / 1000);
        if (samples.size() < 16000 / 2) { // < 0.5s of audio — accidental tap
            flashStatus(tr("Recording too short"));
            return;
        }
        runTranscription(std::move(samples), durationSec, QString());
    }
}

void MainWindow::runTranscription(std::vector<float> samples, int durationSec, const QString &clipId)
{
    const QString id = clipId.isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)
        : clipId;

    if (clipId.isEmpty())
        AudioClipStore::save(id, samples);

    const QString modelPath = m_models->localPath(m_models->activeModelId());

    QVariantMap job;
    job[QStringLiteral("clipId")] = id;
    job[QStringLiteral("durationSec")] = durationSec;
    job[QStringLiteral("isRetry")] = !clipId.isEmpty();
    m_transcribeWatcher.setProperty("job", job);

    m_transcribing = true;
    flashStatus(tr("Transcribing…"));

    // WhisperEngine is only ever touched from inside this task while
    // m_transcribing guards against a second one starting.
    WhisperEngine *engine = m_engine.get();
    m_transcribeWatcher.setFuture(QtConcurrent::run(
        [engine, modelPath, samples = std::move(samples)]() -> QString {
            if (engine->loadedPath() != modelPath) {
                if (!engine->loadModel(modelPath))
                    return QString();
            }
            return engine->transcribe(samples, QStringLiteral("en"));
        }));
}

void MainWindow::addCard(const Transcript &t, bool atTop)
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
    connect(card, &TranscriptCard::retryRequested, this, &MainWindow::retranscribe);
    connect(card, &TranscriptCard::playRequested, this, &MainWindow::playClip);

    const int insertPos = atTop ? 1 : m_listLayout->count() - 1; // slot 0 is the empty state
    m_listLayout->insertWidget(insertPos, card);
    atTop ? m_cards.prepend(card) : m_cards.append(card);

    refreshEmptyState();
}

void MainWindow::playClip(const QString &id)
{
    if (!AudioClipStore::exists(id)) {
        flashStatus(tr("No audio kept for this transcript"));
        return;
    }
    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(AudioClipStore::path(id)));
    m_player->play();
}

void MainWindow::retranscribe(const QString &id)
{
    if (m_transcribing) {
        flashStatus(tr("Still transcribing the previous recording…"));
        return;
    }
    if (!ensureModelReady())
        return;

    std::vector<float> samples = AudioClipStore::load(id);
    if (samples.empty()) {
        flashStatus(tr("No audio kept for this transcript"));
        return;
    }

    int durationSec = 0;
    for (auto *card : std::as_const(m_cards)) {
        if (card->data().id == id) {
            durationSec = card->data().durationSec;
            break;
        }
    }
    runTranscription(std::move(samples), durationSec, id);
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
    AudioClipStore::remove(id);
    persist();
    refreshEmptyState();
    flashStatus(tr("Deleted"));
}

void MainWindow::persist()
{
    QList<Transcript> list;
    list.reserve(m_cards.size());
    for (auto *card : std::as_const(m_cards))
        list.append(card->data());
    TranscriptStore::save(list);
}

void MainWindow::applyFilter(const QString &needle)
{
    for (auto *card : std::as_const(m_cards))
        card->setVisible(card->matches(needle));
    refreshEmptyState();
}

void MainWindow::refreshEmptyState()
{
    m_emptyState->setVisible(m_cards.isEmpty());
}

void MainWindow::flashStatus(const QString &message)
{
    m_status->setText(message);
    m_statusTimer->start(3000);
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
    if (urls.isEmpty() || !urls.first().isLocalFile())
        return;
    if (m_transcribing) {
        flashStatus(tr("Still transcribing the previous recording…"));
        return;
    }
    if (!ensureModelReady())
        return;

    flashStatus(tr("Decoding %1…").arg(urls.first().fileName()));

    auto *decoder = new AudioFileDecoder(urls.first().toLocalFile(), this);
    connect(decoder, &AudioFileDecoder::finished, this,
            [this](std::vector<float> samples, const QString &error) {
                if (!error.isEmpty() || samples.empty()) {
                    flashStatus(tr("Could not decode file: %1").arg(error));
                    return;
                }
                const int durationSec = int(samples.size() / 16000);
                runTranscription(std::move(samples), durationSec, QString());
            });
    decoder->start();
}
