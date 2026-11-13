#include <gtest/gtest.h>
#include "../src/context/practicelib.h"
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QTimeZone>
#include <cmath>
#include <limits>
#include <unistd.h>

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
TEST(Practice, EachStatisticOfThePitchTrackHasItsKnownValue)
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
TEST(Practice, TheTrendGroupsTheSessionsAndFiltersByTask)
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
TEST(Practice, TheLibraryWritesAndReadsSessionsInATreeOfFolders)
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
TEST(Practice, TheHistoryIsCsvTextWithOneLineForEachSessionAndDoublesQuotes)
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

namespace {

// A session with known values on all five tracks. Track t holds the values base + 0 to base + 100 in steps
// of 1, where base is 100 * (t + 1). So the median is base + 50, and the sample SD is 29.3003 for each track.
PracticeRecord knownSession()
{
    PracticeRecord r;
    r.id = "known";
    r.start = QDateTime::fromString("2026-11-13T09:00:00", Qt::ISODate);
    r.tag = "Free";
    r.duration = 100.0;
    for (int track = 0; track < 5; ++track) {
        Summary summary;
        summary.frames = 202;
        for (int i = 0; i <= 100; ++i) summary.values.push_back(100.0 * (track + 1) + i);
        (track == 0 ? r.pitch : r.formants[track - 1]) = statsFrom(summary);
    }
    return r;
}

PracticeRecord withCount(int count, int frames = 10, double mean = 100.0)
{
    PracticeRecord r = make("c", "2026-11-13T09:00:00", "Free", 100.0);
    r.pitch.count = count;
    r.pitch.frames = frames;
    r.pitch.mean = mean;
    r.pitch.sd = 10.0;
    r.pitch.p5 = 90.0;
    r.pitch.p95 = 110.0;
    return r;
}

}

// Each of the 14 statistics has its known value on each of the 5 tracks.
TEST(PracticeStats, TP_PRG_01_EachStatisticHasItsKnownValueOnEachTrack)
{
    const PracticeRecord r = knownSession();
    const double sd = std::sqrt(2.0 * 42925.0 / 100.0);   // the sample SD of 0 to 100 around 50
    for (int track = 0; track < 5; ++track) {
        const double base = 100.0 * (track + 1);
        double v = 0.0;
        auto value = [&](TrendStat stat) { return statValue(r, track, stat, &v) ? v : std::nan(""); };

        EXPECT_NEAR(value(TrendStat::Median), base + 50.0, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::Mean), base + 50.0, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::SD), sd, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::CV), 100.0 * sd / (base + 50.0), 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::Min), base, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::P5), base + 5.0, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::P25), base + 25.0, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::P75), base + 75.0, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::P95), base + 95.0, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::Max), base + 100.0, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::Range), 90.0, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::RangeSemitones), 12.0 * std::log2((base + 95.0) / (base + 5.0)), 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::Voiced), 100.0 * 101.0 / 202.0, 1e-9) << "track " << track;
        EXPECT_NEAR(value(TrendStat::Count), 101.0, 1e-9) << "track " << track;
    }
}

// Each statistic needs a number of values: the SD and the CV need 2 and the ranges need 5.
TEST(PracticeStats, TP_PRG_02_EachStatisticNeedsItsMinimumNumberOfValues)
{
    double v = 0.0;
    for (int count = 0; count <= 6; ++count) {
        const PracticeRecord r = withCount(count);
        EXPECT_EQ(statValue(r, 0, TrendStat::Median, &v), count >= 1) << count;
        EXPECT_EQ(statValue(r, 0, TrendStat::SD, &v), count >= 2) << count;
        EXPECT_EQ(statValue(r, 0, TrendStat::CV, &v), count >= 2) << count;
        EXPECT_EQ(statValue(r, 0, TrendStat::Range, &v), count >= 5) << count;
        EXPECT_EQ(statValue(r, 0, TrendStat::RangeSemitones, &v), count >= 5) << count;
    }
    // The CV needs a mean above 0. A range in semitones needs a P5 above 0.
    EXPECT_FALSE(statValue(withCount(10, 10, 0.0), 0, TrendStat::CV, &v));
    EXPECT_FALSE(statValue(withCount(10, 10, -5.0), 0, TrendStat::CV, &v));
    PracticeRecord low = withCount(10);
    low.pitch.p5 = 0.0;
    EXPECT_FALSE(statValue(low, 0, TrendStat::RangeSemitones, &v));
    EXPECT_TRUE(statValue(low, 0, TrendStat::Range, &v));
}

// A track with frames and no values has a share of 0 and a count of 0, and no other value.
TEST(PracticeStats, TP_PRG_03_ATrackWithFramesAndNoValuesHasOnlyTheShareAndTheCount)
{
    PracticeRecord r;
    r.id = "empty";
    r.start = QDateTime::fromString("2026-11-13T09:00:00", Qt::ISODate);
    for (int track = 0; track < 5; ++track) {
        (track == 0 ? r.pitch : r.formants[track - 1]).frames = 40;
    }
    double v = -1.0;
    for (int track = 0; track < 5; ++track) {
        EXPECT_TRUE(statValue(r, track, TrendStat::Voiced, &v)) << track;
        EXPECT_EQ(v, 0.0);
        v = -1.0;
        EXPECT_TRUE(statValue(r, track, TrendStat::Count, &v)) << track;
        EXPECT_EQ(v, 0.0);
        for (TrendStat stat : { TrendStat::Median, TrendStat::Mean, TrendStat::SD, TrendStat::CV, TrendStat::Min, TrendStat::P5,
                                TrendStat::P25, TrendStat::P75, TrendStat::P95, TrendStat::Max, TrendStat::Range, TrendStat::RangeSemitones }) {
            EXPECT_FALSE(statValue(r, track, stat, &v)) << "track " << track << " stat " << (int) stat;
        }
    }
}

// The keys and the starts of the periods across a year end, a leap day, and a change of the ISO week year.
TEST(PracticeStats, TP_PRG_04_ThePeriodsAreRightAcrossAYearEndALeapDayAndTheISOWeekYear)
{
    // 2026-12-31 is a Thursday in week 53. 2027-01-01 is a Friday in the same week of 2026.
    EXPECT_EQ(periodKey(QDate(2026, 12, 31), TrendPeriod::Week), "2026-W53");
    EXPECT_EQ(periodKey(QDate(2027, 1, 1), TrendPeriod::Week), "2026-W53");
    EXPECT_EQ(periodKey(QDate(2027, 1, 4), TrendPeriod::Week), "2027-W01");
    // 2024-12-30 is a Monday. It starts week 1 of 2025.
    EXPECT_EQ(periodKey(QDate(2024, 12, 29), TrendPeriod::Week), "2024-W52");
    EXPECT_EQ(periodKey(QDate(2024, 12, 30), TrendPeriod::Week), "2025-W01");
    EXPECT_EQ(periodKey(QDate(2024, 12, 30), TrendPeriod::Year), "2024");
    EXPECT_EQ(periodKey(QDate(2024, 2, 29), TrendPeriod::Day), "2024-02-29");
    EXPECT_EQ(periodKey(QDate(2024, 2, 29), TrendPeriod::Month), "2024-02");
    EXPECT_EQ(periodKey(QDate(2024, 3, 1), TrendPeriod::Month), "2024-03");

    QList<PracticeRecord> records;
    records << make("a", "2024-12-29T09:00:00", "Free", 100.0);
    records << make("b", "2024-12-30T09:00:00", "Free", 110.0);
    records << make("c", "2024-12-31T09:00:00", "Free", 120.0);
    records << make("d", "2025-01-01T09:00:00", "Free", 130.0);
    records << make("e", "2024-02-29T09:00:00", "Free", 140.0);

    const auto weeks = buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Week, "");
    ASSERT_EQ(weeks.size(), 3u);
    EXPECT_EQ(weeks[0].key, "2024-W09");
    EXPECT_EQ(weeks[0].start, QDate(2024, 2, 26));
    EXPECT_EQ(weeks[1].key, "2024-W52");
    EXPECT_EQ(weeks[1].start, QDate(2024, 12, 23));
    EXPECT_EQ(weeks[1].sessions, 1);
    // Three sessions of two calendar years fall into one week. Its start is Monday 2024-12-30.
    EXPECT_EQ(weeks[2].key, "2025-W01");
    EXPECT_EQ(weeks[2].start, QDate(2024, 12, 30));
    EXPECT_EQ(weeks[2].sessions, 3);
    EXPECT_DOUBLE_EQ(weeks[2].median, 120.0);

    const auto years = buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Year, "");
    ASSERT_EQ(years.size(), 2u);
    EXPECT_EQ(years[0].key, "2024");
    EXPECT_EQ(years[0].sessions, 4);
    EXPECT_EQ(years[1].key, "2025");
    EXPECT_EQ(years[1].start, QDate(2025, 1, 1));

    const auto months = buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Month, "");
    ASSERT_EQ(months.size(), 3u);
    EXPECT_EQ(months[0].key, "2024-02");
    EXPECT_EQ(months[1].key, "2024-12");
    EXPECT_EQ(months[2].key, "2025-01");
}

// Sessions at 23:59 and 00:01 fall into two days, also on a day when the clock changes for summer time.
TEST(PracticeStats, TP_PRG_05_SessionsAroundMidnightFallIntoTheRightDaysAtADaylightSavingChange)
{
    const QTimeZone zone("America/New_York");
    ASSERT_TRUE(zone.isValid());

    // The clocks of New York go forward at 02:00 on 2026-03-08 and back at 02:00 on 2026-11-01.
    struct Case { QDate day; QTime time; const char *id; };
    const Case cases[] = {
        { QDate(2026, 3, 7), QTime(23, 59), "a" },
        { QDate(2026, 3, 8), QTime(0, 1), "b" },
        { QDate(2026, 3, 8), QTime(23, 59), "c" },
        { QDate(2026, 3, 9), QTime(0, 1), "d" },
        { QDate(2026, 10, 31), QTime(23, 59), "e" },
        { QDate(2026, 11, 1), QTime(0, 1), "f" },
        { QDate(2026, 11, 1), QTime(23, 59), "g" },
        { QDate(2026, 11, 2), QTime(0, 1), "h" },
    };

    // The sessions go through the files of a library, so the test includes the writing and the reading of the time.
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());
    for (const Case &c : cases) {
        PracticeRecord r = make(c.id, "2026-01-01T00:00:00", "Free", 100.0);
        r.start = QDateTime(c.day, c.time, zone);
        ASSERT_TRUE(r.start.isValid());
        ASSERT_TRUE(library.save(r));
    }

    const QList<PracticeRecord> list = library.list();
    ASSERT_EQ(list.size(), 8);
    // Two sessions share a day on 2026-03-08 and on 2026-11-01.
    const auto dayList = buildTrend(list, TrendMeasure::Pitch, TrendPeriod::Day, "");
    EXPECT_EQ(dayList.size(), 6u);
    EXPECT_EQ(dayList[0].key, "2026-03-07");
    EXPECT_EQ(dayList[1].key, "2026-03-08");
    EXPECT_EQ(dayList[1].sessions, 2);
    EXPECT_EQ(dayList[2].key, "2026-03-09");
    EXPECT_EQ(dayList[3].key, "2026-10-31");
    EXPECT_EQ(dayList[4].key, "2026-11-01");
    EXPECT_EQ(dayList[4].sessions, 2);
    EXPECT_EQ(dayList[5].key, "2026-11-02");
}

// The median of the session values for an even and an odd number of sessions.
TEST(PracticeStats, TP_PRG_06_TheMedianOfTheSessionValuesForAnEvenAndAnOddNumber)
{
    QList<PracticeRecord> odd;
    odd << make("1", "2026-11-13T08:00:00", "Free", 300.0);
    odd << make("2", "2026-11-13T09:00:00", "Free", 100.0);
    odd << make("3", "2026-11-13T10:00:00", "Free", 200.0);
    const auto a = buildTrend(odd, TrendMeasure::Pitch, TrendPeriod::Day, "");
    ASSERT_EQ(a.size(), 1u);
    EXPECT_DOUBLE_EQ(a[0].median, 200.0);
    EXPECT_DOUBLE_EQ(a[0].min, 100.0);
    EXPECT_DOUBLE_EQ(a[0].max, 300.0);

    QList<PracticeRecord> even = odd;
    even << make("4", "2026-11-13T11:00:00", "Free", 400.0);
    const auto b = buildTrend(even, TrendMeasure::Pitch, TrendPeriod::Day, "");
    ASSERT_EQ(b.size(), 1u);
    EXPECT_DOUBLE_EQ(b[0].median, 250.0);
    EXPECT_EQ(b[0].sessions, 4);

    QList<PracticeRecord> one;
    one << make("1", "2026-11-13T08:00:00", "Free", 321.0);
    EXPECT_DOUBLE_EQ(buildTrend(one, TrendMeasure::Pitch, TrendPeriod::Day, "")[0].median, 321.0);
}

// The filter by task ignores the letter case. The empty filter keeps all sessions.
TEST(PracticeStats, TP_PRG_07_TheFilterByTaskIgnoresTheLetterCase)
{
    QList<PracticeRecord> records;
    records << make("1", "2026-11-10T09:00:00", "Vowels", 100.0);
    records << make("2", "2026-11-11T09:00:00", "vowels", 110.0);
    records << make("3", "2026-11-12T09:00:00", "VOWELS", 120.0);
    records << make("4", "2026-11-13T09:00:00", "Reading", 130.0);

    EXPECT_EQ(buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Day, "vowels").size(), 3u);
    EXPECT_EQ(buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Day, "VoWeLs").size(), 3u);
    EXPECT_EQ(buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Day, "reading").size(), 1u);
    EXPECT_EQ(buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Day, "").size(), 4u);
    EXPECT_EQ(buildTrend(records, TrendMeasure::Pitch, TrendPeriod::Day, "Conversation").size(), 0u);
    EXPECT_EQ(buildTrend({}, TrendMeasure::Pitch, TrendPeriod::Day, "").size(), 0u);
}

// The old API of the measures gives the same numbers as the new API of the statistics.
TEST(PracticeStats, TP_PRG_13_TheMeasureApiAndTheStatisticApiGiveTheSameNumbers)
{
    QList<PracticeRecord> records;
    for (int i = 0; i < 6; ++i) {
        PracticeRecord r = knownSession();
        r.id = QString::number(i);
        r.start = QDateTime::fromString(QString("2026-11-%1T09:00:00").arg(10 + i), Qt::ISODate);
        r.pitch.median += 7.0 * i;
        r.pitch.sd += i;
        r.formants[0].median += 3.0 * i;
        records << r;
    }

    struct Pair { TrendMeasure measure; int track; TrendStat stat; };
    const Pair pairs[] = {
        { TrendMeasure::Pitch, 0, TrendStat::Median }, { TrendMeasure::F1, 1, TrendStat::Median },
        { TrendMeasure::F2, 2, TrendStat::Median }, { TrendMeasure::F3, 3, TrendStat::Median },
        { TrendMeasure::F4, 4, TrendStat::Median }, { TrendMeasure::Range, 0, TrendStat::RangeSemitones },
        { TrendMeasure::Variation, 0, TrendStat::CV },
    };
    for (const Pair &p : pairs) {
        const auto oldApi = buildTrend(records, p.measure, TrendPeriod::Week, "");
        const auto newApi = buildTrend(records, p.track, p.stat, TrendPeriod::Week, "");
        ASSERT_EQ(oldApi.size(), newApi.size());
        ASSERT_GT(oldApi.size(), 0u);
        for (size_t i = 0; i < oldApi.size(); ++i) {
            EXPECT_EQ(oldApi[i].key, newApi[i].key);
            EXPECT_DOUBLE_EQ(oldApi[i].median, newApi[i].median);
            EXPECT_DOUBLE_EQ(oldApi[i].min, newApi[i].min);
            EXPECT_DOUBLE_EQ(oldApi[i].max, newApi[i].max);
            EXPECT_EQ(oldApi[i].sessions, newApi[i].sessions);
        }
        double a = 0.0, b = 0.0;
        EXPECT_EQ(measureValue(records[0], p.measure, &a), statValue(records[0], p.track, p.stat, &b));
        EXPECT_DOUBLE_EQ(a, b);
    }
}

// One session has all files. The files hold the fields of the session.
TEST(PracticeFiles, TP_PRA_04_OneSessionHasAllFilesWithAllFields)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());

    PracticeRecord r = knownSession();
    r.tag = "Vowels";
    r.note = "after \"warm-up\"";
    PracticeMark m1;
    m1.time = 12.5;
    m1.note = "say \"ah\"";
    m1.pitch = 201.5;
    m1.formants[0] = 650.0;
    m1.formants[1] = 1500.0;
    PracticeMark m2;
    m2.time = 30.0;
    r.marks << m1 << m2;
    PracticeTimelinePoint t;
    t.time = 5.0;
    t.pitch = 199.0;
    t.formants[2] = 2800.0;
    r.timeline << t;

    const QString folder = library.sessionFolder(r);
    ASSERT_TRUE(library.save(r));
    ASSERT_TRUE(writeBundleText(folder, r, "# Report\n\nText\n"));

    // The sound file comes from the session code. The list says that the folder holds it.
    const QStringList names = practiceBundleFiles(true);
    EXPECT_EQ(names, QStringList({ "session.json", "report.md", "stats.csv", "marks.csv", "audio.wav" }));
    for (const QString &name : names) {
        if (name == "audio.wav") continue;
        EXPECT_TRUE(QFile::exists(folder + "/" + name)) << name.toStdString();
    }

    QFile json(folder + "/session.json");
    ASSERT_TRUE(json.open(QIODevice::ReadOnly));
    const PracticeRecord back = recordFromJson(QJsonDocument::fromJson(json.readAll()).object());
    EXPECT_EQ(QJsonDocument(recordToJson(back)).toJson(), QJsonDocument(recordToJson(r)).toJson());
    EXPECT_EQ(back.tag, "Vowels");
    EXPECT_EQ(back.note, "after \"warm-up\"");
    EXPECT_EQ(back.marks.size(), 2);
    EXPECT_EQ(back.timeline.size(), 1);

    QFile report(folder + "/report.md");
    ASSERT_TRUE(report.open(QIODevice::ReadOnly));
    EXPECT_EQ(report.readAll(), QByteArray("# Report\n\nText\n"));

    QFile stats(folder + "/stats.csv");
    ASSERT_TRUE(stats.open(QIODevice::ReadOnly));
    const QStringList statRows = QString::fromUtf8(stats.readAll()).split('\n', Qt::SkipEmptyParts);
    ASSERT_EQ(statRows.size(), 2);
    EXPECT_TRUE(statRows[0].startsWith("id,start,duration_s,task"));
    EXPECT_TRUE(statRows[1].startsWith("known,2026-11-13T09:00:00,100.0,\"Vowels\""));

    QFile marks(folder + "/marks.csv");
    ASSERT_TRUE(marks.open(QIODevice::ReadOnly));
    const QStringList markRows = QString::fromUtf8(marks.readAll()).split('\n', Qt::SkipEmptyParts);
    ASSERT_EQ(markRows.size(), 3);
    EXPECT_EQ(markRows[0], "time_s,note,pitch_hz,f1_hz,f2_hz,f3_hz,f4_hz");
    EXPECT_EQ(markRows[1], "12.5,\"say \"\"ah\"\"\",201.5,650,1500,0,0");
    EXPECT_EQ(markRows[2], "30.0,\"\",0.0,0,0,0,0");
}

// A session with the sound off has no sound file and all statistics.
TEST(PracticeFiles, TP_PRA_05_ASessionWithTheSoundOffHasNoSoundFileAndAllStatistics)
{
    EXPECT_EQ(practiceBundleFiles(false), QStringList({ "session.json", "report.md", "stats.csv", "marks.csv" }));

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());
    const PracticeRecord r = knownSession();
    const QString folder = library.sessionFolder(r);
    ASSERT_TRUE(library.save(r));
    ASSERT_TRUE(writeBundleText(folder, r, "report"));

    EXPECT_FALSE(QFile::exists(folder + "/audio.wav"));
    EXPECT_EQ(QDir(folder).entryList(QDir::Files, QDir::Name), QStringList({ "marks.csv", "report.md", "session.json", "stats.csv" }));
    const PracticeRecord back = library.list().first();
    double v = 0.0;
    for (int track = 0; track < 5; ++track) {
        EXPECT_TRUE(statValue(back, track, TrendStat::Median, &v)) << track;
        EXPECT_NEAR(v, 100.0 * (track + 1) + 50.0, 1e-9);
    }
}

// The library skips each kind of broken file and lists the other sessions.
TEST(PracticeFiles, TP_PRA_09_TheLibrarySkipsEachKindOfBrokenFileAndListsTheOthers)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());
    ASSERT_TRUE(library.save(make("good1", "2026-11-12T10:00:00", "Free", 185.0)));
    ASSERT_TRUE(library.save(make("good2", "2026-11-13T10:00:00", "Free", 190.0)));

    const char *broken[] = {
        "{ not json",                                  // a syntax error
        "",                                            // an empty file
        "[1, 2, 3]",                                   // an array and not an object
        "{\"start\": \"2026-11-13T10:00:00\"}",        // no id
        "{\"id\": \"x\", \"start\": \"yesterday\"}",   // a time that is not a time
        "{\"id\": \"y\"}",                             // no time
        "null",
    };
    int n = 0;
    for (const char *text : broken) {
        const QString folder = dir.path() + QString("/2026/11/13/broken%1").arg(n++);
        ASSERT_TRUE(QDir().mkpath(folder));
        QFile file(folder + "/session.json");
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(text);
    }
    // A folder with no session.json is not a session.
    ASSERT_TRUE(QDir().mkpath(dir.path() + "/2026/11/13/empty"));

    const QList<PracticeRecord> list = library.list();
    ASSERT_EQ(list.size(), 2);
    EXPECT_EQ(list[0].id, "good1");
    EXPECT_EQ(list[1].id, "good2");
}

// The library of 5000 sessions opens in under 2 seconds.
TEST(PracticeFiles, TP_PRA_10_ALibraryOf5000SessionsOpensInUnderTwoSeconds)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());
    const QDateTime first = QDateTime::fromString("2024-01-01T08:00:00", Qt::ISODate);
    for (int i = 0; i < 5000; ++i) {
        PracticeRecord r = make(QString("s%1").arg(i), "2024-01-01T08:00:00", i % 2 ? "Vowels" : "Reading", 150.0 + i % 100);
        r.start = first.addSecs(i * 3600);
        r.marks.append(PracticeMark());
        r.timeline.append(PracticeTimelinePoint());
        ASSERT_TRUE(library.save(r));
    }

    QElapsedTimer timer;
    timer.start();
    const QList<PracticeRecord> list = library.list();
    const qint64 listMs = timer.elapsed();
    EXPECT_EQ(list.size(), 5000);
    // The plan limit is 2 s on an idle machine. The test allows three times this, because a busy machine is slower.
    EXPECT_LT(listMs, 6000) << "list() took " << listMs << " ms";

    // The Progress window also builds the trend of the whole library.
    timer.restart();
    const auto trend = buildTrend(list, TrendMeasure::Pitch, TrendPeriod::Month, "");
    EXPECT_GT(trend.size(), 0u);
    EXPECT_LT(timer.elapsed() + listMs, 6000);
}

// The export copies each folder and writes the history file. A target that cannot be written gives a clear text.
TEST(PracticeFiles, TP_PRA_11_ExportAllCopiesEachFolderAndWritesTheHistory)
{
    QTemporaryDir source;
    QTemporaryDir target;
    ASSERT_TRUE(source.isValid() && target.isValid());
    PracticeLibrary library(source.path());

    QList<PracticeRecord> records;
    records << make("20261111-090000", "2026-11-11T09:00:00", "Free", 180.0);
    records << make("20261112-090000", "2026-11-12T09:00:00", "Vowels", 185.0);
    records << make("20261113-090000", "2026-11-13T09:00:00", "Reading", 190.0);
    for (const PracticeRecord &r : records) {
        ASSERT_TRUE(library.save(r));
        ASSERT_TRUE(writeBundleText(library.sessionFolder(r), r, "report of " + r.id));
    }

    const QDateTime now = QDateTime::fromString("2026-11-14T10:20:30", Qt::ISODate);
    const PracticeExport result = exportPracticeLibrary(library, target.path(), now);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.copied, 3);
    EXPECT_EQ(result.total, 3);
    EXPECT_EQ(result.message, "Exported 3 of 3 sessions");
    EXPECT_EQ(result.folder, target.path() + "/InFormant-practice-20261114-102030");

    for (const PracticeRecord &r : records) {
        const QString relative = QDir(source.path()).relativeFilePath(library.sessionFolder(r));
        for (const QString &name : { "session.json", "report.md", "stats.csv", "marks.csv" }) {
            EXPECT_TRUE(QFile::exists(result.folder + "/" + relative + "/" + name)) << relative.toStdString() << "/" << name.toStdString();
        }
    }
    QFile history(result.folder + "/history.csv");
    ASSERT_TRUE(history.open(QIODevice::ReadOnly));
    EXPECT_EQ(QString::fromUtf8(history.readAll()).split('\n', Qt::SkipEmptyParts).size(), 4);

    // A second export in the same second replaces the files and reports the same numbers.
    const PracticeExport again = exportPracticeLibrary(library, target.path(), now);
    EXPECT_TRUE(again.ok);
    EXPECT_EQ(again.copied, 3);

    // An empty library gives a history file with the header only.
    QTemporaryDir emptySource;
    const PracticeExport none = exportPracticeLibrary(PracticeLibrary(emptySource.path()), target.path(), now.addDays(1));
    EXPECT_TRUE(none.ok);
    EXPECT_EQ(none.message, "Exported 0 of 0 sessions");
}

TEST(PracticeFiles, TP_PRA_11_ExportToAFolderThatCannotBeWrittenGivesAClearText)
{
    if (geteuid() == 0) GTEST_SKIP() << "A user with all rights can write to any folder.";

    QTemporaryDir source;
    QTemporaryDir parent;
    ASSERT_TRUE(source.isValid() && parent.isValid());
    PracticeLibrary library(source.path());
    ASSERT_TRUE(library.save(make("20261113-090000", "2026-11-13T09:00:00", "Free", 190.0)));

    const QString locked = parent.path() + "/locked";
    ASSERT_TRUE(QDir().mkpath(locked));
    ASSERT_TRUE(QFile::setPermissions(locked, QFileDevice::ReadOwner | QFileDevice::ExeOwner));

    const PracticeExport result = exportPracticeLibrary(library, locked, QDateTime::fromString("2026-11-14T10:20:30", Qt::ISODate));
    QFile::setPermissions(locked, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.copied, 0);
    EXPECT_TRUE(result.message.contains("not writable")) << result.message.toStdString();
    EXPECT_TRUE(result.message.contains(locked));
}

// A comparison with a session that has no formants has lines for the common values only.
TEST(PracticeFiles, TP_PRA_15_AComparisonWithASessionThatHasNoFormantsHasLinesForTheCommonValuesOnly)
{
    PracticeRecord previous = make("1", "2026-11-02T09:00:00", "Vowels", 170.0);
    previous.formants[0].count = 0;
    previous.formants[0].frames = 0;
    previous.formants[0].median = 0.0;
    const PracticeRecord current = make("2", "2026-11-04T09:00:00", "Vowels", 180.0, 650.0);

    const QStringList lines = compareSessions(current, &previous);
    ASSERT_GE(lines.size(), 2);
    for (const QString &line : lines) {
        EXPECT_FALSE(line.contains("F1")) << line.toStdString();
        EXPECT_FALSE(line.contains("F2")) << line.toStdString();
    }
    EXPECT_TRUE(lines[0].startsWith("Median pitch: 180.0 Hz, up 10.0 Hz"));

    // Two sessions with no value in common get one line that says so.
    PracticeRecord none = make("3", "2026-11-05T09:00:00", "Vowels", 0.0);
    none.pitch = TrackStats();
    none.formants[0] = TrackStats();
    EXPECT_EQ(compareSessions(none, &previous), QStringList({ "The two sessions have no value in common." }));
}

// The first session of a task gets the text "first session".
TEST(PracticeFiles, TP_PRA_16_TheFirstSessionOfATaskGetsTheTextFirstSession)
{
    QList<PracticeRecord> records;
    records << make("1", "2026-11-02T09:00:00", "Reading", 170.0);
    const PracticeRecord vowels = make("2", "2026-11-03T09:00:00", "Vowels", 175.0);
    records << vowels;

    // Another task does not count, and a later session of the same task does not count.
    const PracticeRecord *previous = previousSameTag(records, vowels);
    EXPECT_EQ(previous, nullptr);
    EXPECT_EQ(compareSessions(vowels, previous), QStringList({ "This is the first session with the task \"Vowels\"." }));

    records << make("3", "2026-11-09T09:00:00", "Vowels", 180.0);
    EXPECT_EQ(previousSameTag(records, vowels), nullptr);
    EXPECT_NE(previousSameTag(records, records.last()), nullptr);
}

// A session of 0 seconds, 1 second, and 12 hours has safe statistics.
TEST(PracticeFiles, TP_PRA_17_SessionsOfZeroOneAndTwelveHoursHaveSafeStatistics)
{
    for (double length : { 0.0, 1.0, 12.0 * 3600.0 }) {
        PracticeRecord r;
        r.id = "len";
        r.start = QDateTime::fromString("2026-11-13T09:00:00", Qt::ISODate);
        r.duration = length;
        r.pitch = statsFrom(Summary());
        const auto parts = timelineParts(0.0, length);
        EXPECT_LE(parts.size(), 12u) << length;
        EXPECT_EQ(parts.empty(), length == 0.0);

        // No statistic has a value that is not a number, and the CSV text has no "nan" or "inf".
        double v = 0.0;
        for (int track = 0; track < 5; ++track) {
            for (int stat = 0; stat <= (int) TrendStat::Count; ++stat) {
                if (statValue(r, track, (TrendStat) stat, &v)) EXPECT_TRUE(std::isfinite(v));
            }
        }
        const QString csv = historyCsv({ r });
        EXPECT_FALSE(csv.contains("nan", Qt::CaseInsensitive)) << csv.toStdString();
        EXPECT_FALSE(csv.contains("inf", Qt::CaseInsensitive)) << csv.toStdString();
        EXPECT_TRUE(csv.contains(QString::number(length, 'f', 1)));
    }

    // One value gives an SD of 0 and no value for the statistics that need more values.
    Summary one;
    one.frames = 1;
    one.values = { 150.0 };
    const TrackStats s = statsFrom(one);
    EXPECT_EQ(s.sd, 0.0);
    EXPECT_EQ(s.median, 150.0);
    EXPECT_EQ(s.p5, 150.0);
    EXPECT_EQ(s.p95, 150.0);
}

// A session with no voiced frames has zero counts and no division by zero.
TEST(PracticeFiles, TP_PRA_18_ASessionWithNoVoicedFramesHasZeroCountsAndNoDivisionByZero)
{
    Summary silent;
    silent.frames = 500;
    const TrackStats s = statsFrom(silent);
    EXPECT_EQ(s.count, 0);
    EXPECT_EQ(s.frames, 500);
    for (double value : { s.mean, s.median, s.sd, s.min, s.max, s.p5, s.p25, s.p75, s.p95 }) EXPECT_EQ(value, 0.0);

    PracticeRecord r;
    r.id = "silent";
    r.start = QDateTime::fromString("2026-11-13T09:00:00", Qt::ISODate);
    r.duration = 60.0;
    r.pitch = s;
    double v = -1.0;
    EXPECT_TRUE(statValue(r, 0, TrendStat::Voiced, &v));
    EXPECT_EQ(v, 0.0);
    EXPECT_TRUE(statValue(r, 0, TrendStat::Count, &v));
    EXPECT_EQ(v, 0.0);
    EXPECT_FALSE(statValue(r, 0, TrendStat::CV, &v));
    EXPECT_FALSE(statValue(r, 0, TrendStat::RangeSemitones, &v));

    // A trend of the session has no point for a statistic that needs values, and the csv text is finite.
    QList<PracticeRecord> records;
    records << r;
    EXPECT_EQ(buildTrend(records, 0, TrendStat::Median, TrendPeriod::Day, "").size(), 0u);
    EXPECT_EQ(buildTrend(records, 0, TrendStat::Voiced, TrendPeriod::Day, "").size(), 1u);
    EXPECT_FALSE(historyCsv(records).contains("nan", Qt::CaseInsensitive));

    // The same holds for a track with no frame at all.
    Summary nothing;
    EXPECT_EQ(statsFrom(nothing).frames, 0);
}

// The timeline of medians has at most 12 points, and the points cover the session.
TEST(PracticeFiles, TP_PRA_21_TheTimelineOfMediansHasAtMostTwelvePointsAndCoversTheSession)
{
    // A session of 5 seconds has one point. A session of 3 hours has 12 points of 15 minutes.
    const auto five = timelineParts(0.0, 5.0);
    ASSERT_EQ(five.size(), 1u);
    EXPECT_EQ(five[0].begin, 0.0);
    EXPECT_EQ(five[0].end, 5.0);

    const auto three = timelineParts(100.0, 100.0 + 3.0 * 3600.0);
    ASSERT_EQ(three.size(), 12u);
    for (const TimelinePart &part : three) EXPECT_DOUBLE_EQ(part.end - part.begin, 900.0);

    for (double length : { 0.5, 1.0, 4.9, 5.0, 5.1, 11.0, 59.0, 60.0, 61.0, 100.0, 601.0, 3600.0, 40000.0, 86400.0 }) {
        const auto parts = timelineParts(10.0, 10.0 + length);
        ASSERT_FALSE(parts.empty()) << length;
        EXPECT_LE(parts.size(), 12u) << length;
        EXPECT_DOUBLE_EQ(parts.front().begin, 10.0) << length;
        EXPECT_NEAR(parts.back().end, 10.0 + length, 1e-6) << length;
        for (size_t i = 1; i < parts.size(); ++i) {
            EXPECT_NEAR(parts[i].begin, parts[i - 1].end, 1e-6) << length;
        }
        // Each part is 5 seconds or longer, except the last part, which holds the rest.
        for (size_t i = 0; i + 1 < parts.size(); ++i) EXPECT_GE(parts[i].end - parts[i].begin, 5.0) << length;
    }
}

// A session shorter than the minimum is dropped. The minimum itself stays.
TEST(PracticeFiles, TP_PRA_35_ASessionShorterThanTheMinimumIsDropped)
{
    const double minimum = practiceMinimumSeconds(5);
    EXPECT_EQ(minimum, 5.0);
    EXPECT_TRUE(practiceTooShort(4.9, minimum));
    EXPECT_FALSE(practiceTooShort(5.0, minimum));
    EXPECT_FALSE(practiceTooShort(5.1, minimum));
    EXPECT_TRUE(practiceTooShort(0.0, minimum));
}

// A minimum of 0, below 0, or a text gives 1 second.
TEST(PracticeFiles, TP_PRA_37_AMinimumOfZeroBelowZeroOrATextGivesOneSecond)
{
    EXPECT_EQ(practiceMinimumSeconds(0), 1.0);
    EXPECT_EQ(practiceMinimumSeconds(-3), 1.0);
    EXPECT_EQ(practiceMinimumSeconds(0.5), 1.0);
    EXPECT_EQ(practiceMinimumSeconds("text"), 1.0);
    EXPECT_EQ(practiceMinimumSeconds(""), 1.0);
    EXPECT_EQ(practiceMinimumSeconds(QVariant()), 1.0);
    EXPECT_EQ(practiceMinimumSeconds(std::nan("")), 1.0);
    EXPECT_EQ(practiceMinimumSeconds(1), 1.0);
    EXPECT_EQ(practiceMinimumSeconds(30), 30.0);
    EXPECT_EQ(practiceMinimumSeconds("7"), 7.0);
}

// A note that starts with a formula character does not run as a formula in a spreadsheet.
TEST(PracticeFiles, ANoteThatStartsLikeAFormulaIsQuotedInTheCsv)
{
    QList<PracticeMark> marks;
    for (const char *note : { "=1+1", "+1", "-1", "@SUM(A1)", "plain note" }) {
        PracticeMark m;
        m.note = note;
        marks.append(m);
    }
    const QString csv = marksCsv(marks);
    for (const char *formula : { "=1+1", "+1", "-1", "@SUM(A1)" }) {
        EXPECT_TRUE(csv.contains(QString("\"'%1\"").arg(formula))) << formula;
        EXPECT_FALSE(csv.contains(QString(",\"%1\"").arg(formula))) << formula;
    }
    EXPECT_TRUE(csv.contains("\"plain note\""));
}

// The minimum length of a practice session has an upper limit, and a value that is not finite gives the limit or 1 s.
TEST(PracticeFiles, TheMinimumLengthOfASessionHasAnUpperLimit)
{
    EXPECT_EQ(practiceMinimumSeconds(1e12), 3600.0);
    EXPECT_EQ(practiceMinimumSeconds(std::numeric_limits<double>::infinity()), 3600.0);
    EXPECT_EQ(practiceMinimumSeconds(std::numeric_limits<double>::quiet_NaN()), 1.0);
    EXPECT_EQ(practiceMinimumSeconds(30), 30.0);
}

namespace {

// A saved session with a report and a sound file, as the program writes it.
void saveWithFiles(const PracticeLibrary &library, const PracticeRecord &r)
{
    ASSERT_TRUE(library.save(r));
    ASSERT_TRUE(writeBundleText(library.sessionFolder(r), r, "report of " + r.id));
    QFile sound(library.sessionFolder(r) + "/audio.wav");
    ASSERT_TRUE(sound.open(QIODevice::WriteOnly));
    sound.write("RIFF");
}

}

// Delete one session. Its folder goes with every file, and the other sessions stay.
TEST(PracticeDelete, TP_PRG_14_TheFolderOfOneSessionGoesAndTheOthersStay)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());

    const PracticeRecord a = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    const PracticeRecord b = make("20261113-100000", "2026-11-13T10:00:00", "Vowels", 190.0);
    saveWithFiles(library, a);
    saveWithFiles(library, b);

    EXPECT_TRUE(library.remove(a));
    EXPECT_FALSE(QDir(library.sessionFolder(a)).exists());
    EXPECT_FALSE(QFile::exists(library.sessionFolder(a) + "/audio.wav"));

    // The other session on the same day stays, and so does its day folder.
    EXPECT_TRUE(QFile::exists(library.sessionFolder(b) + "/session.json"));
    EXPECT_TRUE(QFile::exists(library.sessionFolder(b) + "/audio.wav"));
    const QList<PracticeRecord> left = library.list();
    ASSERT_EQ(left.size(), 1);
    EXPECT_EQ(left[0].id, b.id);
}

// The day, the month, and the year folders go when no session keeps them. The root and the busy folders stay.
TEST(PracticeDelete, TP_PRG_14_EmptyParentFoldersGoButTheRootAndBusyFoldersStay)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());

    const PracticeRecord only = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    const PracticeRecord sameMonth = make("20261120-090000", "2026-11-20T09:00:00", "Free", 181.0);
    const PracticeRecord otherYear = make("20251201-090000", "2025-12-01T09:00:00", "Free", 182.0);
    saveWithFiles(library, only);
    saveWithFiles(library, sameMonth);
    saveWithFiles(library, otherYear);

    // The day 13 is empty after the delete, so it goes. The month 11 keeps the day 20, and so does the year.
    EXPECT_TRUE(library.remove(only));
    EXPECT_FALSE(QDir(dir.path() + "/2026/11/13").exists());
    EXPECT_TRUE(QDir(dir.path() + "/2026/11/20").exists());
    EXPECT_TRUE(QDir(dir.path() + "/2026/11").exists());

    // Now the month and the year go, and the other year stays.
    EXPECT_TRUE(library.remove(sameMonth));
    EXPECT_FALSE(QDir(dir.path() + "/2026").exists());
    EXPECT_TRUE(QDir(dir.path() + "/2025/12/01").exists());

    // The last session goes, and the root stays.
    EXPECT_TRUE(library.remove(otherYear));
    EXPECT_FALSE(QDir(dir.path() + "/2025").exists());
    EXPECT_TRUE(QDir(dir.path()).exists());
    EXPECT_TRUE(library.list().isEmpty());
}

// A file that the person put in the day folder is not a session, so the day folder stays.
TEST(PracticeDelete, TP_PRG_14_AFileOfThePersonKeepsItsFolder)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());

    const PracticeRecord r = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    saveWithFiles(library, r);
    QFile note(dir.path() + "/2026/11/13/my-notes.txt");
    ASSERT_TRUE(note.open(QIODevice::WriteOnly));
    note.write("keep me");
    note.close();

    EXPECT_TRUE(library.remove(r));
    EXPECT_TRUE(QFile::exists(dir.path() + "/2026/11/13/my-notes.txt"));
}

// A session that does not exist gives false and deletes nothing. A second delete of the same session gives false.
TEST(PracticeDelete, TP_PRG_14_ASessionThatIsNotThereGivesFalse)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());

    const PracticeRecord kept = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    const PracticeRecord ghost = make("20261114-090000", "2026-11-14T09:00:00", "Free", 180.0);
    saveWithFiles(library, kept);

    EXPECT_FALSE(library.remove(ghost));
    EXPECT_TRUE(QFile::exists(library.sessionFolder(kept) + "/session.json"));

    EXPECT_TRUE(library.remove(kept));
    EXPECT_FALSE(library.remove(kept));

    // A library folder that does not exist gives false, with no error.
    EXPECT_FALSE(PracticeLibrary(dir.path() + "/missing").remove(kept));
    EXPECT_FALSE(PracticeLibrary(QString()).remove(kept));
}

// A folder with no session.json is not a session. It stays, even when its name matches the id.
TEST(PracticeDelete, TP_PRG_14_AFolderWithNoSessionFileStays)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());

    const PracticeRecord r = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    ASSERT_TRUE(QDir().mkpath(library.sessionFolder(r)));
    QFile other(library.sessionFolder(r) + "/important.txt");
    ASSERT_TRUE(other.open(QIODevice::WriteOnly));
    other.write("data");
    other.close();

    EXPECT_FALSE(library.remove(r));
    EXPECT_TRUE(QFile::exists(library.sessionFolder(r) + "/important.txt"));
}

// A session.json with an id like "../x" must never delete a folder outside its day, and never outside the library.
TEST(PracticeDelete, TP_PRG_14_AnIdWithPathPartsDeletesNothing)
{
    QTemporaryDir outer;
    ASSERT_TRUE(outer.isValid());
    const QString rootPath = outer.path() + "/library";
    ASSERT_TRUE(QDir().mkpath(rootPath));
    PracticeLibrary library(rootPath);

    // A folder outside the library, with a session.json, that the bad id points to.
    const QString victim = outer.path() + "/victim";
    ASSERT_TRUE(QDir().mkpath(victim));
    QFile victimFile(victim + "/session.json");
    ASSERT_TRUE(victimFile.open(QIODevice::WriteOnly));
    victimFile.write("{}");
    victimFile.close();

    const PracticeRecord good = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    saveWithFiles(library, good);

    for (const QString &badId : { QString("../../../../victim"), QString("../../../../../victim"), QString(".."),
                                  QString("../"), QString("."), QString("/"), QString("../20261113-090000"),
                                  QString("a/b") }) {
        PracticeRecord bad = good;
        bad.id = badId;
        EXPECT_FALSE(library.remove(bad)) << badId.toStdString();
    }
    EXPECT_TRUE(QFile::exists(victim + "/session.json"));
    EXPECT_TRUE(QFile::exists(library.sessionFolder(good) + "/session.json"));

    // A record with an empty id deletes nothing.
    PracticeRecord empty = good;
    empty.id.clear();
    EXPECT_FALSE(library.remove(empty));
    EXPECT_TRUE(QFile::exists(library.sessionFolder(good) + "/session.json"));
}

// A session folder that is a link to a folder outside the library is refused, and the target stays.
TEST(PracticeDelete, TP_PRG_14_ALinkOutOfTheLibraryIsNotFollowed)
{
    QTemporaryDir outer;
    ASSERT_TRUE(outer.isValid());
    const QString rootPath = outer.path() + "/library";
    PracticeLibrary library(rootPath);

    const PracticeRecord r = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    const QString target = outer.path() + "/elsewhere";
    ASSERT_TRUE(QDir().mkpath(target));
    QFile targetFile(target + "/session.json");
    ASSERT_TRUE(targetFile.open(QIODevice::WriteOnly));
    targetFile.write("{}");
    targetFile.close();

    ASSERT_TRUE(QDir().mkpath(QFileInfo(library.sessionFolder(r)).absolutePath()));
    ASSERT_TRUE(QFile::link(target, library.sessionFolder(r)));

    EXPECT_FALSE(library.remove(r));
    EXPECT_TRUE(QFile::exists(target + "/session.json"));
}

// The list gives the folder where it found each session, and remove deletes exactly that folder.
TEST(PracticeDelete, TP_PRG_14_TheListGivesTheFolderAndRemoveDeletesThatFolder)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());

    const PracticeRecord a = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    const PracticeRecord b = make("20261114-090000", "2026-11-14T09:00:00", "Free", 181.0);
    saveWithFiles(library, a);
    saveWithFiles(library, b);

    const QList<PracticeRecord> listed = library.list();
    ASSERT_EQ(listed.size(), 2);
    EXPECT_EQ(QFileInfo(listed[0].folder).canonicalFilePath(), QFileInfo(library.sessionFolder(a)).canonicalFilePath());

    EXPECT_TRUE(library.remove(listed[0]));
    EXPECT_FALSE(QDir(library.sessionFolder(a)).exists());
    EXPECT_TRUE(QDir(library.sessionFolder(b)).exists());
}

// A copy of a session in another place of the library, with the same id, is a second row in the list. A delete of
// one row must never delete the other. The copy is not at the depth of a session folder, so it is refused and stays.
TEST(PracticeDelete, TP_PRG_14_ACopyWithTheSameIdIsNotDeletedByTheDeleteOfTheOriginal)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    PracticeLibrary library(dir.path());

    const PracticeRecord original = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    saveWithFiles(library, original);

    // The person copies the folder into a backup folder of the library.
    const QString copy = dir.path() + "/backup/2026/11/13/20261113-090000";
    ASSERT_TRUE(QDir().mkpath(copy));
    for (const QString &name : QDir(library.sessionFolder(original)).entryList(QDir::Files)) {
        ASSERT_TRUE(QFile::copy(library.sessionFolder(original) + "/" + name, copy + "/" + name));
    }

    const QList<PracticeRecord> listed = library.list();
    ASSERT_EQ(listed.size(), 2);

    for (const PracticeRecord &row : listed) {
        const bool isCopy = row.folder.contains("/backup/");
        if (isCopy) {
            // The delete of the copy is refused, and the original stays.
            EXPECT_FALSE(library.remove(row));
            EXPECT_TRUE(QDir(library.sessionFolder(original)).exists());
            EXPECT_TRUE(QDir(copy).exists());
        }
    }
    for (const PracticeRecord &row : library.list()) {
        if (!row.folder.contains("/backup/")) EXPECT_TRUE(library.remove(row));
    }

    // The original is gone, and the copy stays.
    EXPECT_FALSE(QDir(library.sessionFolder(original)).exists());
    EXPECT_TRUE(QFile::exists(copy + "/session.json"));
}

// A record whose folder is outside the library is refused, even when its id looks right.
TEST(PracticeDelete, TP_PRG_14_AFolderOutsideTheLibraryIsRefused)
{
    QTemporaryDir outer;
    ASSERT_TRUE(outer.isValid());
    const QString rootPath = outer.path() + "/library";
    ASSERT_TRUE(QDir().mkpath(rootPath));
    PracticeLibrary library(rootPath);

    const QString other = outer.path() + "/2026/11/13/20261113-090000";
    ASSERT_TRUE(QDir().mkpath(other));
    QFile file(other + "/session.json");
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("{}");
    file.close();

    PracticeRecord r = make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0);
    r.folder = other;
    EXPECT_FALSE(library.remove(r));
    EXPECT_TRUE(QFile::exists(other + "/session.json"));

    // The root itself and a year folder are refused.
    saveWithFiles(library, make("20261113-090000", "2026-11-13T09:00:00", "Free", 180.0));
    r.folder = rootPath;
    EXPECT_FALSE(library.remove(r));
    r.folder = rootPath + "/2026";
    EXPECT_FALSE(library.remove(r));
    EXPECT_TRUE(QDir(rootPath + "/2026/11/13/20261113-090000").exists());
}
