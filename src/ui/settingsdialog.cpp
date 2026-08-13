#include "settingsdialog.h"

#include "globalhotkey.h"
#include "modelcatalog.h"
#include "modelmanager.h"
#include "theme.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(ModelManager *models, GlobalHotkey *hotkey, QWidget *parent)
    : QDialog(parent)
    , m_models(models)
    , m_hotkey(hotkey)
{
    setWindowTitle(tr("Settings"));
    setMinimumWidth(520);
    setStyleSheet(Theme::styleSheet() + QStringLiteral(R"(
QDialog { background: #1A1A1D; }
QRadioButton { spacing: 8px; font-size: 14px; }
QRadioButton:disabled { color: #5E5E66; }
QProgressBar {
    background: #26262A;
    border: none;
    border-radius: 4px;
    height: 8px;
    text-align: center;
    font-size: 10px;
    color: transparent;
}
QProgressBar::chunk { background: #0A84FF; border-radius: 4px; }
QPushButton {
    background: #2A2A30;
    border: 1px solid #3A3A42;
    border-radius: 8px;
    padding: 5px 14px;
    font-size: 13px;
}
QPushButton:hover { background: #34343C; }
QPushButton:disabled { color: #5E5E66; background: #222226; }
)"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 16);
    layout->setSpacing(14);

    auto *heading = new QLabel(tr("Transcription model"), this);
    heading->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600;"));
    layout->addWidget(heading);

    auto *sub = new QLabel(tr("Models are downloaded once and stored locally. "
                              "Larger models are more accurate but slower."), this);
    sub->setObjectName(QStringLiteral("cardMeta"));
    sub->setWordWrap(true);
    layout->addWidget(sub);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(1, 1);

    int row = 0;
    for (const ModelInfo &info : ModelCatalog::all()) {
        Row r;

        r.active = new QRadioButton(info.label, this);
        r.active->setToolTip(info.description);
        connect(r.active, &QRadioButton::toggled, this, [this, id = info.id](bool on) {
            if (on)
                m_models->setActiveModelId(id);
        });
        grid->addWidget(r.active, row, 0);

        r.size = new QLabel(ModelCatalog::humanSize(info.approxBytes), this);
        r.size->setObjectName(QStringLiteral("cardMeta"));
        grid->addWidget(r.size, row, 1, Qt::AlignLeft);

        r.progress = new QProgressBar(this);
        r.progress->setRange(0, 100);
        r.progress->setVisible(false);
        r.progress->setFixedWidth(110);
        grid->addWidget(r.progress, row, 2);

        r.action = new QPushButton(this);
        r.action->setFixedWidth(96);
        connect(r.action, &QPushButton::clicked, this, [this, id = info.id] {
            if (m_models->isDownloading(id))
                m_models->cancelDownload(id);
            else if (m_models->isDownloaded(id))
                onDeleteClicked(id);
            else
                onDownloadClicked(id);
        });
        grid->addWidget(r.action, row, 3);

        m_rows.insert(info.id, r);
        ++row;
    }
    layout->addLayout(grid);

    // --- global shortcut ---------------------------------------------------
    auto *hotkeyHeading = new QLabel(tr("Global shortcut"), this);
    hotkeyHeading->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600;"));
    layout->addWidget(hotkeyHeading);

    auto *hotkeyRow = new QHBoxLayout;
    hotkeyRow->setSpacing(10);

    m_hotkeyEdit = new QKeySequenceEdit(this);
    m_hotkeyEdit->setMaximumSequenceLength(1);
    m_hotkeyEdit->setClearButtonEnabled(true);
    m_hotkeyEdit->setKeySequence(m_hotkey->sequence());
    m_hotkeyEdit->setStyleSheet(QStringLiteral(
        "QLineEdit{background:#26262A;border:1px solid #3A3A42;border-radius:8px;padding:6px 10px;}"));
    connect(m_hotkeyEdit, &QKeySequenceEdit::editingFinished,
            this, &SettingsDialog::onHotkeyEdited);
    hotkeyRow->addWidget(m_hotkeyEdit, 1);

    auto *resetBtn = new QPushButton(tr("Reset"), this);
    connect(resetBtn, &QPushButton::clicked, this, [this] {
        m_hotkeyEdit->setKeySequence(GlobalHotkey::defaultSequence());
        onHotkeyEdited();
    });
    hotkeyRow->addWidget(resetBtn);
    layout->addLayout(hotkeyRow);

    m_hotkeyStatus = new QLabel(this);
    m_hotkeyStatus->setObjectName(QStringLiteral("cardMeta"));
    m_hotkeyStatus->setWordWrap(true);
    m_hotkeyStatus->setText(tr("Click the field, then press the keys. Needs at least one "
                               "modifier (Ctrl/Cmd/Alt/Shift). Works while the app is in "
                               "the background."));
    layout->addWidget(m_hotkeyStatus);

    layout->addStretch(1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(m_models, &ModelManager::downloadProgress, this, &SettingsDialog::onProgress);
    connect(m_models, &ModelManager::downloadFinished, this, &SettingsDialog::onFinished);

    for (const ModelInfo &info : ModelCatalog::all())
        refreshRow(info.id);
}

void SettingsDialog::refreshRow(const QString &id)
{
    auto it = m_rows.find(id);
    if (it == m_rows.end())
        return;
    Row &r = *it;

    const bool downloaded = m_models->isDownloaded(id);
    const bool downloading = m_models->isDownloading(id);

    r.active->setEnabled(downloaded);
    // Block the toggled->setActiveModelId round-trip while reflecting state.
    r.active->blockSignals(true);
    r.active->setChecked(downloaded && m_models->activeModelId() == id);
    r.active->blockSignals(false);

    r.progress->setVisible(downloading);
    if (!downloading)
        r.progress->setValue(0);

    if (downloading)
        r.action->setText(tr("Cancel"));
    else if (downloaded)
        r.action->setText(tr("Delete"));
    else
        r.action->setText(tr("Download"));
}

void SettingsDialog::onHotkeyEdited()
{
    const QKeySequence seq = m_hotkeyEdit->keySequence();

    // Empty (user hit the clear button) — keep the current one.
    if (seq.isEmpty()) {
        m_hotkeyEdit->setKeySequence(m_hotkey->sequence());
        return;
    }
    if (seq == m_hotkey->sequence())
        return;

    // A global hotkey without modifiers would swallow plain typing systemwide.
    if (seq[0].keyboardModifiers() == Qt::NoModifier) {
        m_hotkeyStatus->setText(tr("⚠ Needs at least one modifier key (Ctrl/Cmd/Alt/Shift)."));
        m_hotkeyEdit->setKeySequence(m_hotkey->sequence());
        return;
    }

    if (m_hotkey->setSequence(seq)) {
        QSettings().setValue(QStringLiteral("globalHotkey"),
                             seq.toString(QKeySequence::PortableText));
        m_hotkeyStatus->setText(tr("Shortcut set to %1.").arg(m_hotkey->comboLabel()));
    } else {
        m_hotkeyStatus->setText(tr("⚠ Couldn't register that combo (unsupported key or "
                                   "already taken by another app). Kept %1.")
                                    .arg(m_hotkey->comboLabel()));
        m_hotkeyEdit->setKeySequence(m_hotkey->sequence());
    }
}

void SettingsDialog::onDownloadClicked(const QString &id)
{
    m_models->download(id);
    refreshRow(id);
}

void SettingsDialog::onDeleteClicked(const QString &id)
{
    if (m_models->activeModelId() == id) {
        QMessageBox::information(this, tr("Model in use"),
                                 tr("This model is currently selected. "
                                    "Pick another model before deleting it."));
        return;
    }
    m_models->removeDownloaded(id);
    refreshRow(id);
}

void SettingsDialog::onProgress(const QString &id, qint64 received, qint64 total)
{
    auto it = m_rows.find(id);
    if (it == m_rows.end())
        return;

    if (total <= 0) {
        if (const ModelInfo *info = ModelCatalog::find(id))
            total = info->approxBytes; // server didn't say — use catalog estimate
    }
    if (total > 0)
        it->progress->setValue(static_cast<int>(received * 100 / total));
}

void SettingsDialog::onFinished(const QString &id, bool ok, const QString &error)
{
    refreshRow(id);
    if (!ok && !error.contains(QStringLiteral("canceled"), Qt::CaseInsensitive)) {
        QMessageBox::warning(this, tr("Download failed"),
                             tr("Could not download the model:\n%1").arg(error));
    }
}
