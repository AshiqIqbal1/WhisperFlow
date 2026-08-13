#ifndef TRANSCRIPTSTORE_H
#define TRANSCRIPTSTORE_H

#include "transcriptcard.h"

#include <QList>

// Flat JSON file on disk: <AppLocalDataLocation>/transcripts.json
// Deliberately not a database — this is a personal transcript list, not
// data at a scale that needs one. Swap for SQLite later if it grows.
namespace TranscriptStore {

QList<Transcript> load();
void save(const QList<Transcript> &transcripts);

} // namespace TranscriptStore

#endif // TRANSCRIPTSTORE_H
