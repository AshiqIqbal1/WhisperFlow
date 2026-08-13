#include "audiofiledecoder.h"

#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QUrl>

AudioFileDecoder::AudioFileDecoder(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_path(filePath)
    , m_decoder(new QAudioDecoder(this))
{
    // Ask the decoder to hand us whisper's format directly; Qt resamples
    // and downmixes internally so we don't have to.
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);
    m_decoder->setAudioFormat(format);

    connect(m_decoder, &QAudioDecoder::bufferReady, this, [this] {
        const QAudioBuffer buffer = m_decoder->read();
        const float *data = buffer.constData<float>();
        m_samples.insert(m_samples.end(), data, data + buffer.sampleCount());
    });

    connect(m_decoder, &QAudioDecoder::finished, this, [this] {
        emit finished(std::move(m_samples), QString());
        deleteLater();
    });

    // error is overloaded (getter + signal) — qOverload picks the signal.
    connect(m_decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this,
            [this](QAudioDecoder::Error) {
                emit finished({}, m_decoder->errorString());
                deleteLater();
            });
}

void AudioFileDecoder::start()
{
    m_decoder->setSource(QUrl::fromLocalFile(m_path));
    m_decoder->start();
}
