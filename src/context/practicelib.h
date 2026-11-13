#ifndef MAIN_CONTEXT_PRACTICELIB_H
#define MAIN_CONTEXT_PRACTICELIB_H

#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
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

    // The statistic of one track that a trend can show. The track is 0 for the pitch and 1 to 4 for F1 to F4.
    // Range is P95 minus P5 in Hz. RangeSemitones is the same range in semitones. Voiced is the share of
    // frames with a value in percent. Count is the number of values.
    enum class TrendStat { Median, Mean, SD, CV, Min, P5, P25, P75, P95, Max, Range, RangeSemitones, Voiced, Count };

    // The names are "median", "mean", "sd", "cv", "min", "p5", "p25", "p75", "p95", "max", "range",
    // "range_st", "voiced", and "count". An unknown name gives the median.
    TrendStat trendStatFromName(const QString &name);

    // The track of a name: "pitch" is 0 and "f1" to "f4" are 1 to 4. An unknown name gives 0.
    int trendTrackFromName(const QString &name);

    // The value of a statistic of one track in one session. The result is false when the session has no value.
    bool statValue(const PracticeRecord &record, int track, TrendStat stat, double *value);

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

    // The same trend for any statistic of any track.
    std::vector<TrendPoint> buildTrend(const QList<PracticeRecord> &records, int track, TrendStat stat,
                                       TrendPeriod period, const QString &tagFilter);

    // The newest session before this one that has the same task. The result is null when none exists.
    const PracticeRecord *previousSameTag(const QList<PracticeRecord> &records, const PracticeRecord &current);

    // Lines of text that compare a session with the previous one of the same task.
    QStringList compareSessions(const PracticeRecord &current, const PracticeRecord *previous);

    // One line for each session, as CSV text, for the export of the whole history.
    QString historyCsv(const QList<PracticeRecord> &records);

    // The shortest session that the program keeps, in seconds. The option holds a number. A value of 0,
    // a negative value, a text that is not a number, or no value gives 1 second.
    double practiceMinimumSeconds(const QVariant &option);

    // A session is too short when its length is below the minimum. A length equal to the minimum stays.
    bool practiceTooShort(double lengthSeconds, double minimumSeconds);

    // One part of the session for a point of the timeline. The times are in seconds of the session.
    struct TimelinePart {
        double begin = 0.0;
        double end = 0.0;
    };

    // The parts of a session from t0 to t1 that have equal length. A part is 5 seconds or longer, and a
    // session has at most 12 parts. A session with no length has no part.
    std::vector<TimelinePart> timelineParts(double t0, double t1);

    // The files in the folder of a session. The sound file is in the list only when the person keeps the sound.
    QStringList practiceBundleFiles(bool keepSound);

    // The marks as CSV text with a header line.
    QString marksCsv(const QList<PracticeMark> &marks);

    class PracticeLibrary;

    // Write report.md, stats.csv, and marks.csv into a folder that exists. The result is false when a file fails.
    bool writeBundleText(const QString &folder, const PracticeRecord &record, const QString &reportText);

    // The result of the export of a whole library.
    struct PracticeExport {
        bool ok = false;
        int copied = 0;         // the sessions that were copied without an error
        int total = 0;          // all sessions of the library
        QString folder;         // the folder that holds the copy
        QString message;        // the text for the status line
    };

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

    // Copy each session folder and a history.csv into a new folder InFormant-practice-<time> in the target.
    // The result says how many sessions were copied. A target that cannot be written gives ok false.
    PracticeExport exportPracticeLibrary(const PracticeLibrary &library, const QString &target, const QDateTime &now);

}

#endif // MAIN_CONTEXT_PRACTICELIB_H
