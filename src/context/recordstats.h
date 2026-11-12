#ifndef MAIN_CONTEXT_RECORD_STATS_H
#define MAIN_CONTEXT_RECORD_STATS_H

#include <QDateTime>
#include <QString>
#include <QStringList>

#include "config.h"
#include "datastore.h"
#include "summary.h"

namespace Main {

    // The statistics of the values of a track from t0 to t1. The frames are all entries, and the values are the ones that have a value.
    Summary summarizeTrack(OptionalTimeTrack<double> &track, double t0, double t1);

    // The description of a recording as a Markdown text: the settings, the statistics of the pitch and
    // the formants for the whole recording, and the statistics for each part of the recording.
    // The times t0 and t1 are in seconds of the session.
    QString buildRecordingReport(Config &config, DataStore &dataStore, double t0, double t1,
                                 const QDateTime &started, const QStringList &files);

}

#endif // MAIN_CONTEXT_RECORD_STATS_H
