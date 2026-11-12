#ifndef MAIN_CONTEXT_PRACTICELIB_H
#define MAIN_CONTEXT_PRACTICELIB_H

#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <vector>

#include "summary.h"

// The data of the practice mode: the record of one session, the library of sessions on the disk, and the
// trends over many sessions. The code has no dependency on the data store or on the window, so a test can run it.

namespace Main {

    // The statistics of one track in one session. The values are in Hz.
    struct TrackStats {
        int count = 0;     // frames with a value
        int frames = 0;    // all frames
        double mean = 0.0;
        double median = 0.0;
        double sd = 0.0;
        double min = 0.0;
        double max = 0.0;
        double p5 = 0.0;
        double p25 = 0.0;
        double p75 = 0.0;
        double p95 = 0.0;
    };

    TrackStats statsFrom(const Summary &summary);

    // A mark that the person set during the session. The values are the medians of the last half second.
    struct PracticeMark {
        double time = 0.0;      // seconds since the start of the session
        QString note;
        double pitch = 0.0;     // 0 means no value
        double formants[4] = { 0.0, 0.0, 0.0, 0.0 };
    };

    // The median of each track in one part of the session.
    struct PracticeTimelinePoint {
        double time = 0.0;      // seconds since the start of the session
        double pitch = 0.0;
        double formants[4] = { 0.0, 0.0, 0.0, 0.0 };
    };

    struct PracticeRecord {
        QString id;             // for example 20261113-091500
        QDateTime start;
        double duration = 0.0;  // seconds
        QString tag;            // the task, for example "Vowels"
        QString note;
        TrackStats pitch;
        TrackStats formants[4];
        QList<PracticeMark> marks;
        QList<PracticeTimelinePoint> timeline;
    };

    QJsonObject recordToJson(const PracticeRecord &record);
    PracticeRecord recordFromJson(const QJsonObject &json);

    // The measures that a trend can show. A value of a session is the median of the track, the range of
    // the pitch from the 5th to the 95th percentile in semitones, or the variation of the pitch in percent.
    enum class TrendMeasure { Pitch, F1, F2, F3, F4, Range, Variation };
    enum class TrendPeriod { Day, Week, Month, Year };

    TrendMeasure trendMeasureFromName(const QString &name);
    TrendPeriod trendPeriodFromName(const QString &name);

    // The value of a measure for one session. The result is false when the session has no value for it.
    bool measureValue(const PracticeRecord &record, TrendMeasure measure, double *value);

    // The key of the period that holds a date, for example "2026-11" for a month. Keys sort in time order.
    QString periodKey(const QDate &date, TrendPeriod period);

    struct TrendPoint {
        QString key;
        QDate start;            // the first day of the period
        double median = 0.0;    // the median of the session values
        double min = 0.0;
        double max = 0.0;
        double minutes = 0.0;   // the time of practice in the period
        int sessions = 0;
    };

    // The trend of a measure. A tag filter keeps only the sessions of one task. An empty filter keeps all.
    // A period with no session that has a value is not in the result.
    std::vector<TrendPoint> buildTrend(const QList<PracticeRecord> &records, TrendMeasure measure,
                                       TrendPeriod period, const QString &tagFilter);

    // The newest session before this one that has the same task. The result is null when none exists.
    const PracticeRecord *previousSameTag(const QList<PracticeRecord> &records, const PracticeRecord &current);

    // Lines of text that compare a session with the previous one of the same task.
    QStringList compareSessions(const PracticeRecord &current, const PracticeRecord *previous);

    // One line for each session, as CSV text, for the export of the whole history.
    QString historyCsv(const QList<PracticeRecord> &records);

    // The folder tree of the sessions: root/YYYY/MM/DD/<id>/session.json.
    class PracticeLibrary {
    public:
        explicit PracticeLibrary(const QString &root);

        QString root() const { return mRoot; }

        // The folder of a session. The function does not create it.
        QString sessionFolder(const PracticeRecord &record) const;

        // Write session.json into the folder of the session. The folder is created if needed.
        bool save(const PracticeRecord &record) const;

        // All sessions, oldest first. A folder with a broken file is skipped.
        QList<PracticeRecord> list() const;

    private:
        QString mRoot;
    };

}

#endif // MAIN_CONTEXT_PRACTICELIB_H
