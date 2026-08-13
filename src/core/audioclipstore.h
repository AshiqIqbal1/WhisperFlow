#ifndef AUDIOCLIPSTORE_H
#define AUDIOCLIPSTORE_H

#include <QString>
#include <vector>

// Keeps the recorded audio for each transcript as a 16kHz mono PCM16 WAV in
// <AppLocalDataLocation>/audio/<id>.wav — that's what makes the play and
// re-transcribe buttons on a card possible after the fact.
namespace AudioClipStore {

QString path(const QString &id);
bool exists(const QString &id);

// samples: mono float32 in [-1,1] at 16kHz (AudioRecorder's output format).
bool save(const QString &id, const std::vector<float> &samples);

// Inverse of save(). Empty vector if the file is missing/corrupt.
std::vector<float> load(const QString &id);

void remove(const QString &id);

} // namespace AudioClipStore

#endif // AUDIOCLIPSTORE_H
