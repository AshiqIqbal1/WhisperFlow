#ifndef AUDIOFILEDECODER_H
#define AUDIOFILEDECODER_H

#include <QObject>
#include <QString>
#include <vector>

class QAudioDecoder;

// Decodes an audio file (wav/mp3/m4a/… — whatever the platform codecs
// handle) into whisper's required format: mono float32 @ 16kHz. Async;
// one instance per file, fire-and-forget:
//
//   auto *dec = new AudioFileDecoder(path, this);
//   connect(dec, &AudioFileDecoder::finished, this, [](auto samples, auto err){...});
//   dec->start();   // deletes itself after finished()
class AudioFileDecoder : public QObject
{
    Q_OBJECT

public:
    explicit AudioFileDecoder(const QString &filePath, QObject *parent = nullptr);

    void start();

signals:
    void finished(std::vector<float> samples, QString error);

private:
    QString m_path;
    QAudioDecoder *m_decoder = nullptr;
    std::vector<float> m_samples;
};

#endif // AUDIOFILEDECODER_H
