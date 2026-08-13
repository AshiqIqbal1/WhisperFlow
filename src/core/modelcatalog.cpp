#include "modelcatalog.h"

namespace {

// Sizes are the real ggml file sizes on huggingface.co/ggerganov/whisper.cpp
// as of this writing — close enough for a progress bar; the download itself
// always trusts the server's real Content-Length, this is only a fallback.
const QList<ModelInfo> kModels = {
    {QStringLiteral("tiny"),
     QStringLiteral("Tiny"),
     QStringLiteral("Fastest, least accurate. Good for quick drafts."),
     QStringLiteral("ggml-tiny.bin"),
     77'700'000},
    {QStringLiteral("base"),
     QStringLiteral("Base"),
     QStringLiteral("Small step up from Tiny, still very fast."),
     QStringLiteral("ggml-base.bin"),
     148'000'000},
    {QStringLiteral("small"),
     QStringLiteral("Small"),
     QStringLiteral("Good balance of speed and accuracy."),
     QStringLiteral("ggml-small.bin"),
     488'000'000},
    {QStringLiteral("medium"),
     QStringLiteral("Medium"),
     QStringLiteral("Noticeably more accurate, noticeably slower."),
     QStringLiteral("ggml-medium.bin"),
     1'534'000'000},
    {QStringLiteral("large-v3-turbo"),
     QStringLiteral("Large (v3 turbo)"),
     QStringLiteral("Best accuracy. Slow on modest hardware."),
     QStringLiteral("ggml-large-v3-turbo.bin"),
     1'625'000'000},
};

} // namespace

const QList<ModelInfo> &ModelCatalog::all()
{
    return kModels;
}

const ModelInfo *ModelCatalog::find(const QString &id)
{
    for (const ModelInfo &m : kModels) {
        if (m.id == id)
            return &m;
    }
    return nullptr;
}

QString ModelCatalog::downloadUrl(const ModelInfo &info)
{
    return QStringLiteral("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/%1")
        .arg(info.filename);
}

QString ModelCatalog::humanSize(qint64 bytes)
{
    const double gb = bytes / 1000.0 / 1000.0 / 1000.0;
    if (gb >= 1.0)
        return QStringLiteral("%1 GB").arg(gb, 0, 'f', 1);
    const double mb = bytes / 1000.0 / 1000.0;
    return QStringLiteral("%1 MB").arg(mb, 0, 'f', 0);
}
