#include "check.h"
#include "../src/context/practicelib.h"
#include <QTemporaryDir>

using namespace Main;

namespace {

PracticeRecord make(const QString &id, const QString &start, const QString &tag, double pitchMedian, double f1 = 600.0, double duration = 120.0)
{
    PracticeRecord r;
    r.id = id;
    r.start = QDateTime::fromString(start, Qt::ISODate);
    r.tag = tag;
    r.duration = duration;
    r.pitch.count = 100;
    r.pitch.frames = 200;
    r.pitch.mean = pitchMedian;
    r.pitch.median = pitchMedian;
    r.pitch.sd = 0.1 * pitchMedian;
    r.pitch.p5 = pitchMedian * 0.8;
    r.pitch.p95 = pitchMedian * 1.25;
    r.formants[0].count = 50;
    r.formants[0].median = f1;
    return r;
}

}

int main()
{
    // The statistics of a summary.
    {
        Summary s;
        s.frames = 10;
        s.values = { 100, 110, 120, 130, 140 };
        const TrackStats t = statsFrom(s);
        CHECK(t.count == 5 && t.frames == 10);
        CHECK_NEAR(t.mean, 120.0, 1e-9);
        CHECK_NEAR(t.median, 120.0, 1e-9);
        CHECK_NEAR(t.sd, std::sqrt(250.0), 1e-9);
        CHECK(t.min == 100.0 && t.max == 140.0);
        const TrackStats none = statsFrom(Summary());
        CHECK(none.count == 0 && none.median == 0.0);
    }

    // A record goes to JSON and back with its marks and timeline.
    {
        PracticeRecord r = make("20261113-090000", "2026-11-13T09:00:00", "Vowels", 180.0);
        PracticeMark m;
        m.time = 12.5;
        m.note = "good ah";
        m.pitch = 181.0;
        m.formants[1] = 1500.0;
        r.marks.append(m);
        PracticeTimelinePoint t;
        t.time = 5.0;
        t.pitch = 179.0;
        t.formants[0] = 610.0;
        r.timeline.append(t);

        const PracticeRecord b = recordFromJson(recordToJson(r));
        CHECK(b.id == r.id && b.tag == "Vowels" && b.start == r.start);
        CHECK_NEAR(b.pitch.median, 180.0, 1e-9);
        CHECK_NEAR(b.formants[0].median, 600.0, 1e-9);
        CHECK(b.marks.size() == 1 && b.marks[0].note == "good ah");
        CHECK_NEAR(b.marks[0].formants[1], 1500.0, 1e-9);
        CHECK(b.timeline.size() == 1);
        CHECK_NEAR(b.timeline[0].formants[0], 610.0, 1e-9);
    }

    // The measures of a session.
    {
        PracticeRecord r = make("a", "2026-11-13T09:00:00", "Free", 200.0);
        double v = 0.0;
        CHECK(measureValue(r, TrendMeasure::Pitch, &v) && v == 200.0);
        CHECK(measureValue(r, TrendMeasure::F1, &v) && v == 600.0);
        CHECK(!measureValue(r, TrendMeasure::F2, &v));
        CHECK(measureValue(r, TrendMeasure::Range, &v));
        CHECK_NEAR(v, 12.0 * std::log2(1.25 / 0.8), 1e-6);
        CHECK(measureValue(r, TrendMeasure::Variation, &v) && std::fabs(v - 10.0) < 1e-9);
    }

    // The keys of the periods sort in time order, and a week follows the ISO rule.
    {
        const QDate d(2026, 11, 13);
        CHECK(periodKey(d, TrendPeriod::Day) == "2026-11-13");
        CHECK(periodKey(d, TrendPeriod::Month) == "2026-11");
        CHECK(periodKey(d, TrendPeriod::Year) == "2026");
        CHECK(periodKey(QDate(2026, 1, 1), TrendPeriod::Week) == "2026-W01");
        CHECK(periodKey(QDate(2024, 12, 30), TrendPeriod::Week) == "2025-W01");
    }

    // The trend groups the sessions, takes the median of the session values, and filters by task.
    {
        QList<PracticeRecord> records;
        records << make("1", "2026-11-02T09:00:00", "Vowels", 170.0, 600.0, 60.0);
        records << make("2", "2026-11-03T09:00:00", "Vowels", 190.0, 600.0, 120.0);
        records << make("3", "2026-11-03T18:00:00", "Vowels", 180.0, 600.0, 60.0);
        records << make("4", "2026-11-03T19:00:00", "Reading", 150.0, 600.0, 600.0);
        records << make("5", "2026-12-01T09:00:00", "Vowels", 200.0, 600.0, 60.0);

        const auto days = buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Day, "Vowels");
        CHECK(days.size() == 3);
        CHECK(days[1].key == "2026-11-03" && days[1].sessions == 2);
        CHECK_NEAR(days[1].median, 185.0, 1e-9);
        CHECK(days[1].min == 180.0 && days[1].max == 190.0);
        CHECK_NEAR(days[1].minutes, 3.0, 1e-9);

        const auto months = buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Month, "vowels");
        CHECK(months.size() == 2);
        CHECK(months[0].sessions == 3 && months[0].start == QDate(2026, 11, 1));
        CHECK_NEAR(months[0].median, 180.0, 1e-9);

        const auto all = buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Year, "");
        CHECK(all.size() == 1 && all[0].sessions == 5);
        CHECK_NEAR(all[0].median, 180.0, 1e-9);

        const auto none = buildTrend(records, TrendMeasure::F2, TrendPeriod::Day, "");
        CHECK(none.empty());
    }

    // The previous session of the same task, and the comparison text.
    {
        QList<PracticeRecord> records;
        records << make("1", "2026-11-02T09:00:00", "Vowels", 170.0);
        records << make("2", "2026-11-03T09:00:00", "Reading", 150.0);
        records << make("3", "2026-11-04T09:00:00", "Vowels", 180.0, 650.0);

        const PracticeRecord *p = previousSameTag(records, records[2]);
        CHECK(p != nullptr && p->id == "1");
        CHECK(previousSameTag(records, records[0]) == nullptr);

        const QStringList first = compareSessions(records[0], nullptr);
        CHECK(first.size() == 1 && first[0].contains("first session"));

        const QStringList lines = compareSessions(records[2], p);
        CHECK(lines.size() >= 3);
        CHECK(lines[0].contains("up 10.0 Hz"));
        CHECK(lines[0].contains("2026-11-02"));
        bool f1 = false;
        for (const QString &l : lines) f1 = f1 || l.contains("Median F1: 650.0 Hz, up 50.0 Hz");
        CHECK(f1);
    }

    // The library writes and reads sessions in a tree of folders. A broken file is skipped.
    {
        QTemporaryDir dir;
        CHECK(dir.isValid());
        PracticeLibrary library(dir.path());
        const PracticeRecord a = make("20261113-100000", "2026-11-13T10:00:00", "Free", 190.0);
        const PracticeRecord b = make("20261112-100000", "2026-11-12T10:00:00", "Free", 185.0);
        CHECK(library.save(a));
        CHECK(library.save(b));
        CHECK(library.sessionFolder(a).endsWith("2026/11/13/20261113-100000"));

        QDir().mkpath(dir.path() + "/2026/11/13/bad");
        QFile bad(dir.path() + "/2026/11/13/bad/session.json");
        CHECK(bad.open(QIODevice::WriteOnly));
        bad.write("{ not json");
        bad.close();

        const QList<PracticeRecord> list = library.list();
        CHECK(list.size() == 2);
        CHECK(list[0].id == "20261112-100000" && list[1].id == "20261113-100000");
    }

    // The history is CSV text with a header and one line for each session. A quote in a note is doubled.
    {
        QList<PracticeRecord> records;
        PracticeRecord r = make("1", "2026-11-02T09:00:00", "Vowels", 170.0);
        r.note = "say \"ah\"";
        records << r;
        const QString csv = historyCsv(records);
        const QStringList rows = csv.split('\n', Qt::SkipEmptyParts);
        CHECK(rows.size() == 2);
        CHECK(rows[0].startsWith("id,start"));
        CHECK(rows[1].contains("\"say \"\"ah\"\"\""));
    }

    return TEST_RESULT();
}
