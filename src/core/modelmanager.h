#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include "modelcatalog.h"

#include <QMap>
#include <QObject>

class QFile;
class QNetworkAccessManager;
class QNetworkReply;

// Owns the on-disk model cache and the "which model is active" setting.
// Downloading is streamed straight to disk (never buffered fully in RAM —
// the large model is 1.5GB+) and survives app restarts: whatever finished
// downloading last time is still there, whatever didn't gets cleaned up.
class ModelManager : public QObject
{
    Q_OBJECT

public:
    explicit ModelManager(QObject *parent = nullptr);

    // Directory models are cached in: <AppLocalDataLocation>/models/
    QString modelsDir() const;

    bool isDownloaded(const QString &id) const;
    QString localPath(const QString &id) const;

    QString activeModelId() const;
    void setActiveModelId(const QString &id);

    bool isDownloading(const QString &id) const;

public slots:
    void download(const QString &id);
    void cancelDownload(const QString &id);
    void removeDownloaded(const QString &id);

signals:
    void downloadProgress(const QString &id, qint64 received, qint64 total);
    void downloadFinished(const QString &id, bool ok, const QString &error);
    void activeModelChanged(const QString &id);

private:
    struct DownloadState
    {
        QNetworkReply *reply = nullptr;
        QFile *file = nullptr;
        QString tmpPath;
    };

    QNetworkAccessManager *m_net = nullptr;
    QMap<QString, DownloadState> m_downloads;
};

#endif // MODELMANAGER_H
