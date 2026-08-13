#include "modelmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

namespace {
constexpr auto kSettingsKey = "activeModelId";
constexpr QLatin1StringView kDefaultModel("base");
}

ModelManager::ModelManager(QObject *parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
{
    QDir().mkpath(modelsDir());
}

QString ModelManager::modelsDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(base).filePath(QStringLiteral("models"));
}

QString ModelManager::localPath(const QString &id) const
{
    const ModelInfo *info = ModelCatalog::find(id);
    if (!info)
        return QString();
    return QDir(modelsDir()).filePath(info->filename);
}

bool ModelManager::isDownloaded(const QString &id) const
{
    const QString path = localPath(id);
    return !path.isEmpty() && QFileInfo::exists(path);
}

QString ModelManager::activeModelId() const
{
    return QSettings().value(kSettingsKey, QString(kDefaultModel)).toString();
}

void ModelManager::setActiveModelId(const QString &id)
{
    if (id == activeModelId())
        return;
    QSettings().setValue(kSettingsKey, id);
    emit activeModelChanged(id);
}

bool ModelManager::isDownloading(const QString &id) const
{
    return m_downloads.contains(id);
}

void ModelManager::download(const QString &id)
{
    if (m_downloads.contains(id))
        return;

    const ModelInfo *info = ModelCatalog::find(id);
    if (!info) {
        emit downloadFinished(id, false, tr("Unknown model \"%1\"").arg(id));
        return;
    }

    QDir().mkpath(modelsDir());

    const QString finalPath = localPath(id);
    const QString tmpPath = finalPath + QStringLiteral(".part");

    auto *file = new QFile(tmpPath, this);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        delete file;
        emit downloadFinished(id, false, tr("Could not write to %1").arg(tmpPath));
        return;
    }

    QNetworkRequest request(QUrl(ModelCatalog::downloadUrl(*info)));
    // Hugging Face serves the actual bytes from a CDN redirect; this policy
    // follows https->https redirects but still refuses a downgrade to http.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_net->get(request);
    m_downloads.insert(id, {reply, file, tmpPath});

    connect(reply, &QNetworkReply::readyRead, this, [this, id] {
        auto it = m_downloads.find(id);
        if (it != m_downloads.end())
            it->file->write(it->reply->readAll());
    });

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, id](qint64 received, qint64 total) {
                emit downloadProgress(id, received, total);
            });

    connect(reply, &QNetworkReply::finished, this, [this, id, finalPath] {
        auto it = m_downloads.find(id);
        if (it == m_downloads.end())
            return; // already cleaned up via cancel

        QNetworkReply *reply = it->reply;
        QFile *file = it->file;
        const QString tmpPath = it->tmpPath;
        const bool ok = reply->error() == QNetworkReply::NoError;
        const QString error = ok ? QString() : reply->errorString();

        file->close();
        if (ok) {
            QFile::remove(finalPath); // in case a previous file is there
            QFile::rename(tmpPath, finalPath);
        } else {
            QFile::remove(tmpPath);
        }

        reply->deleteLater();
        file->deleteLater();
        m_downloads.remove(id);

        emit downloadFinished(id, ok, error);
    });
}

void ModelManager::cancelDownload(const QString &id)
{
    auto it = m_downloads.find(id);
    if (it != m_downloads.end())
        it->reply->abort(); // finished() handler above does the cleanup
}

void ModelManager::removeDownloaded(const QString &id)
{
    QFile::remove(localPath(id));
}
