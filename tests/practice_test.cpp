#include <gtest/gtest.h>
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

// The statistics of a summary.
TEST(Practice, TheStatisticsOfASummary)
{
    Summary s;
    s.frames = 10;
    s.values = { 100, 110, 120, 130, 140 };
    const TrackStats t = statsFrom(s);
    EXPECT_TRUE(t.count == 5 && t.frames == 10);
    EXPECT_NEAR(t.mean, 120.0, 1e-9);
    EXPECT_NEAR(t.median, 120.0, 1e-9);
    EXPECT_NEAR(t.sd, std::sqrt(250.0), 1e-9);
    EXPECT_TRUE(t.min == 100.0 && t.max == 140.0);
    const TrackStats none = statsFrom(Summary());
    EXPECT_TRUE(none.count == 0 && none.median == 0.0);
}

// A record goes to JSON and back with its marks and timeline.
TEST(Practice, ARecordGoesToJSONAndBackWithItsMarksAndTimeline)
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
    EXPECT_TRUE(b.id == r.id && b.tag == "Vowels" && b.start == r.start);
    EXPECT_NEAR(b.pitch.median, 180.0, 1e-9);
    EXPECT_NEAR(b.formants[0].median, 600.0, 1e-9);
    EXPECT_TRUE(b.marks.size() == 1 && b.marks[0].note == "good ah");
    EXPECT_NEAR(b.marks[0].formants[1], 1500.0, 1e-9);
    EXPECT_TRUE(b.timeline.size() == 1);
    EXPECT_NEAR(b.timeline[0].formants[0], 610.0, 1e-9);
}

// The measures of a session.
TEST(Practice, TheMeasuresOfASession)
{
    PracticeRecord r = make("a", "2026-11-13T09:00:00", "Free", 200.0);
    double v = 0.0;
    EXPECT_TRUE(measureValue(r, TrendMeasure::Pitch, &v) && v == 200.0);
    EXPECT_TRUE(measureValue(r, TrendMeasure::F1, &v) && v == 600.0);
    EXPECT_TRUE(!measureValue(r, TrendMeasure::F2, &v));
    EXPECT_TRUE(measureValue(r, TrendMeasure::Range, &v));
    EXPECT_NEAR(v, 12.0 * std::log2(1.25 / 0.8), 1e-6);
    EXPECT_TRUE(measureValue(r, TrendMeasure::Variation, &v) && std::fabs(v - 10.0) < 1e-9);
}

// Each statistic of each track. The helper makes a session with known statistics.
TEST(Practice, EachStatisticOfEachTrackTheHelperMakesASessionWithKnown)
{
    PracticeRecord r = make("s", "2026-11-13T09:00:00", "Free", 200.0);
    r.pitch.count = 80;
    r.pitch.frames = 100;
    r.pitch.mean = 210.0;
    r.pitch.sd = 21.0;
    r.pitch.min = 150.0;
    r.pitch.max = 300.0;
    r.pitch.p5 = 160.0;
    r.pitch.p25 = 190.0;
    r.pitch.p75 = 230.0;
    r.pitch.p95 = 280.0;
    double v = 0.0;
    EXPECT_TRUE(statValue(r, 0, TrendStat::Mean, &v) && v == 210.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::SD, &v) && v == 21.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::CV, &v) && std::fabs(v - 10.0) < 1e-9);
    EXPECT_TRUE(statValue(r, 0, TrendStat::Min, &v) && v == 150.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::Max, &v) && v == 300.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::P5, &v) && v == 160.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::P25, &v) && v == 190.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::P75, &v) && v == 230.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::P95, &v) && v == 280.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::Range, &v) && v == 120.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::RangeSemitones, &v));
    EXPECT_NEAR(v, 12.0 * std::log2(280.0 / 160.0), 1e-9);
    EXPECT_TRUE(statValue(r, 0, TrendStat::Voiced, &v) && v == 80.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::Count, &v) && v == 80.0);
    // A track with no frames has no value. A track with frames and no values has a share and a count.
    EXPECT_TRUE(!statValue(r, 2, TrendStat::Median, &v));
    EXPECT_TRUE(!statValue(r, 2, TrendStat::Voiced, &v));
    r.formants[1].frames = 50;
    EXPECT_TRUE(statValue(r, 2, TrendStat::Voiced, &v) && v == 0.0);
    EXPECT_TRUE(statValue(r, 2, TrendStat::Count, &v) && v == 0.0);
    EXPECT_TRUE(!statValue(r, 2, TrendStat::Mean, &v));
    // A track outside 0 to 4 has no value.
    EXPECT_TRUE(!statValue(r, 5, TrendStat::Mean, &v));
    EXPECT_TRUE(!statValue(r, -1, TrendStat::Mean, &v));
    // The names of the statistics and the tracks.
    EXPECT_TRUE(trendStatFromName("RANGE_ST") == TrendStat::RangeSemitones);
    EXPECT_TRUE(trendStatFromName("nonsense") == TrendStat::Median);
    EXPECT_TRUE(trendTrackFromName("F3") == 3 && trendTrackFromName("pitch") == 0 && trendTrackFromName("x") == 0);
    // The trend of a statistic uses the same grouping as the trend of a measure.
    QList<PracticeRecord> records;
    records << r;
    const auto sd = buildTrend(records, 0, TrendStat::SD, TrendPeriod::Day, "");
    EXPECT_TRUE(sd.size() == 1 && sd[0].median == 21.0);
}

// The keys of the periods sort in time order, and a week follows the ISO rule.
TEST(Practice, TheKeysOfThePeriodsSortInTimeOrderAndAWeekFollowsTheISORule)
{
    const QDate d(2026, 11, 13);
    EXPECT_TRUE(periodKey(d, TrendPeriod::Day) == "2026-11-13");
    EXPECT_TRUE(periodKey(d, TrendPeriod::Month) == "2026-11");
    EXPECT_TRUE(periodKey(d, TrendPeriod::Year) == "2026");
    EXPECT_TRUE(periodKey(QDate(2026, 1, 1), TrendPeriod::Week) == "2026-W01");
    EXPECT_TRUE(periodKey(QDate(2024, 12, 30), TrendPeriod::Week) == "2025-W01");
}

// The trend groups the sessions, takes the median of the session values, and filters by task.
TEST(Practice, TheTrendGroupsTheSessionsTakesTheMedianOfTheSessionValuesAnd)
{
    QList<PracticeRecord> records;
    records << make("1", "2026-11-02T09:00:00", "Vowels", 170.0, 600.0, 60.0);
    records << make("2", "2026-11-03T09:00:00", "Vowels", 190.0, 600.0, 120.0);
    records << make("3", "2026-11-03T18:00:00", "Vowels", 180.0, 600.0, 60.0);
    records << make("4", "2026-11-03T19:00:00", "Reading", 150.0, 600.0, 600.0);
    records << make("5", "2026-12-01T09:00:00", "Vowels", 200.0, 600.0, 60.0);

    const auto days = buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Day, "Vowels");
    EXPECT_TRUE(days.size() == 3);
    EXPECT_TRUE(days[1].key == "2026-11-03" && days[1].sessions == 2);
    EXPECT_NEAR(days[1].median, 185.0, 1e-9);
    EXPECT_TRUE(days[1].min == 180.0 && days[1].max == 190.0);
    EXPECT_NEAR(days[1].minutes, 3.0, 1e-9);

    const auto months = buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Month, "vowels");
    EXPECT_TRUE(months.size() == 2);
    EXPECT_TRUE(months[0].sessions == 3 && months[0].start == QDate(2026, 11, 1));
    EXPECT_NEAR(months[0].median, 180.0, 1e-9);

    const auto all = buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Year, "");
    EXPECT_TRUE(all.size() == 1 && all[0].sessions == 5);
    EXPECT_NEAR(all[0].median, 180.0, 1e-9);

    const auto none = buildTrend(records, TrendMeasure::F2, TrendPeriod::Day, "");
    EXPECT_TRUE(none.empty());
}

// The previous session of the same task, and the comparison text.
TEST(Practice, ThePreviousSessionOfTheSameTaskAndTheComparisonText)
{
    QList<PracticeRecord> records;
    records << make("1", "2026-11-02T09:00:00", "Vowels", 170.0);
    records << make("2", "2026-11-03T09:00:00", "Reading", 150.0);
    records << make("3", "2026-11-04T09:00:00", "Vowels", 180.0, 650.0);

    const PracticeRecord *p = previousSameTag(records, records[2]);
    EXPECT_TRUE(p != nullptr && p->id == "1");
    EXPECT_TRUE(previousSameTag(records, records[0]) == nullptr);

    const QStringList first = compareSessions(records[0], nullptr);
    EXPECT_TRUE(first.size() == 1 && first[0].contains("first session"));

    const QStringList lines = compareSessions(records[2], p);
    EXPECT_TRUE(lines.size() >= 3);
    EXPECT_TRUE(lines[0].contains("up 10.0 Hz"));
    EXPECT_TRUE(lines[0].contains("2026-11-02"));
    bool f1 = false;
    for (const QString &l : lines) f1 = f1 || l.contains("Median F1: 650.0 Hz, up 50.0 Hz");
    EXPECT_TRUE(f1);
}

// The library writes and reads sessions in a tree of folders. A broken file is skipped.
TEST(Practice, TheLibraryWritesAndReadsSessionsInATreeOfFoldersABrokenFile)
{
    QTemporaryDir dir;
    EXPECT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());
    const PracticeRecord a = make("20261113-100000", "2026-11-13T10:00:00", "Free", 190.0);
    const PracticeRecord b = make("20261112-100000", "2026-11-12T10:00:00", "Free", 185.0);
    EXPECT_TRUE(library.save(a));
    EXPECT_TRUE(library.save(b));
    EXPECT_TRUE(library.sessionFolder(a).endsWith("2026/11/13/20261113-100000"));

    QDir().mkpath(dir.path() + "/2026/11/13/bad");
    QFile bad(dir.path() + "/2026/11/13/bad/session.json");
    EXPECT_TRUE(bad.open(QIODevice::WriteOnly));
    bad.write("{ not json");
    bad.close();

    const QList<PracticeRecord> list = library.list();
    EXPECT_TRUE(list.size() == 2);
    EXPECT_TRUE(list[0].id == "20261112-100000" && list[1].id == "20261113-100000");
}

// The history is CSV text with a header and one line for each session. A quote in a note is doubled.
TEST(Practice, TheHistoryIsCSVTextWithAHeaderAndOneLineForEachSessionAQuote)
{
    QList<PracticeRecord> records;
    PracticeRecord r = make("1", "2026-11-02T09:00:00", "Vowels", 170.0);
    r.note = "say \"ah\"";
    records << r;
    const QString csv = historyCsv(records);
    const QStringList rows = csv.split('\n', Qt::SkipEmptyParts);
    EXPECT_TRUE(rows.size() == 2);
    EXPECT_TRUE(rows[0].startsWith("id,start"));
    EXPECT_TRUE(rows[1].contains("\"say \"\"ah\"\"\""));
}
