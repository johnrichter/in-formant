#ifndef MAIN_CONTEXT_RECORD_STATS_H
#define MAIN_CONTEXT_RECORD_STATS_H

#include <QDateTime>
#include <QString>
#include <QStringList>

#include "config.h"
#include "datastore.h"

namespace Main {

    // The description of a recording as a Markdown text: the settings, the statistics of the pitch and
    // the formants for the whole recording, and the statistics for each part of the recording.
    // The times t0 and t1 are in seconds of the session.
    QString buildRecordingReport(Config &config, DataStore &dataStore, double t0, double t1,
                                 const QDateTime &started, const QStringList &files);

}

#endif // MAIN_CONTEXT_RECORD_STATS_H
