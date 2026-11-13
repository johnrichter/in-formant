#include "practicelib.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>
#include <cmath>
#include <map>

using namespace Main;

namespace {

QJsonObject statsToJson(const TrackStats &s)
{
    QJsonObject o;
    o["count"] = s.count;
    o["frames"] = s.frames;
    o["mean"] = s.mean;
    o["median"] = s.median;
    o["sd"] = s.sd;
    o["min"] = s.min;
    o["max"] = s.max;
    o["p5"] = s.p5;
    o["p25"] = s.p25;
    o["p75"] = s.p75;
    o["p95"] = s.p95;
    return o;
}

TrackStats statsFromJson(const QJsonObject &o)
{
    TrackStats s;
    s.count = o["count"].toInt();
    s.frames = o["frames"].toInt();
    s.mean = o["mean"].toDouble();
    s.median = o["median"].toDouble();
    s.sd = o["sd"].toDouble();
    s.min = o["min"].toDouble();
    s.max = o["max"].toDouble();
    s.p5 = o["p5"].toDouble();
    s.p25 = o["p25"].toDouble();
    s.p75 = o["p75"].toDouble();
    s.p95 = o["p95"].toDouble();
    return s;
}

QJsonArray formantArray(const double (&values)[4])
{
    QJsonArray a;
    for (double v : values) a.append(v);
    return a;
}

void readFormants(const QJsonArray &a, double (&values)[4])
{
    for (int i = 0; i < 4; ++i) values[i] = i < a.size() ? a[i].toDouble() : 0.0;
}

double semitones(double low, double high)
{
    return 12.0 * std::log2(high / low);
}

QString csvField(const QString &text)
{
    QString t = text;
    // A spreadsheet runs a field that starts with one of these characters as a formula. A quote stops this.
    if (!t.isEmpty() && QStringLiteral("=+-@\t\r").contains(t.at(0))) t.prepend('\'');
    t.replace('"', "\"\"");
    return "\"" + t + "\"";
}

}

TrackStats Main::statsFrom(const Summary &summary)
{
    TrackStats s;
    s.frames = (int) summary.frames;
    s.count = (int) summary.values.size();
    if (summary.empty()) return s;
    s.mean = summary.mean();
    s.median = summary.median();
    s.sd = summary.sd();
    s.min = summary.values.front();
    s.max = summary.values.back();
    s.p5 = summary.percentile(0.05);
    s.p25 = summary.percentile(0.25);
    s.p75 = summary.percentile(0.75);
    s.p95 = summary.percentile(0.95);
    return s;
}

QJsonObject Main::recordToJson(const PracticeRecord &r)
{
    QJsonObject o;
    o["version"] = 1;
    o["id"] = r.id;
    o["start"] = r.start.toString(Qt::ISODate);
    o["durationSeconds"] = r.duration;
    o["tag"] = r.tag;
    o["note"] = r.note;
    o["pitch"] = statsToJson(r.pitch);
    for (int i = 0; i < 4; ++i) o[QString("f%1").arg(i + 1)] = statsToJson(r.formants[i]);

    QJsonArray marks;
    for (const PracticeMark &m : r.marks) {
        QJsonObject mo;
        mo["time"] = m.time;
        mo["note"] = m.note;
        mo["pitch"] = m.pitch;
        mo["formants"] = formantArray(m.formants);
        marks.append(mo);
    }
    o["marks"] = marks;

    QJsonArray timeline;
    for (const PracticeTimelinePoint &t : r.timeline) {
        QJsonObject to;
        to["time"] = t.time;
        to["pitch"] = t.pitch;
        to["formants"] = formantArray(t.formants);
        timeline.append(to);
    }
    o["timeline"] = timeline;
    return o;
}

PracticeRecord Main::recordFromJson(const QJsonObject &o)
{
    PracticeRecord r;
    r.id = o["id"].toString();
    r.start = QDateTime::fromString(o["start"].toString(), Qt::ISODate);
    r.duration = o["durationSeconds"].toDouble();
    r.tag = o["tag"].toString();
    r.note = o["note"].toString();
    r.pitch = statsFromJson(o["pitch"].toObject());
    for (int i = 0; i < 4; ++i) r.formants[i] = statsFromJson(o[QString("f%1").arg(i + 1)].toObject());

    for (const QJsonValue &v : o["marks"].toArray()) {
        const QJsonObject mo = v.toObject();
        PracticeMark m;
        m.time = mo["time"].toDouble();
        m.note = mo["note"].toString();
        m.pitch = mo["pitch"].toDouble();
        readFormants(mo["formants"].toArray(), m.formants);
        r.marks.append(m);
    }
    for (const QJsonValue &v : o["timeline"].toArray()) {
        const QJsonObject to = v.toObject();
        PracticeTimelinePoint t;
        t.time = to["time"].toDouble();
        t.pitch = to["pitch"].toDouble();
        readFormants(to["formants"].toArray(), t.formants);
        r.timeline.append(t);
    }
    return r;
}

TrendMeasure Main::trendMeasureFromName(const QString &name)
{
    const QString n = name.toLower();
    if (n == "f1") return TrendMeasure::F1;
    if (n == "f2") return TrendMeasure::F2;
    if (n == "f3") return TrendMeasure::F3;
    if (n == "f4") return TrendMeasure::F4;
    if (n == "range") return TrendMeasure::Range;
    if (n == "variation" || n == "cv") return TrendMeasure::Variation;
    return TrendMeasure::Pitch;
}

TrendPeriod Main::trendPeriodFromName(const QString &name)
{
    const QString n = name.toLower();
    if (n == "week") return TrendPeriod::Week;
    if (n == "month") return TrendPeriod::Month;
    if (n == "year") return TrendPeriod::Year;
    return TrendPeriod::Day;
}

TrendStat Main::trendStatFromName(const QString &name)
{
    const QString n = name.toLower();
    if (n == "mean") return TrendStat::Mean;
    if (n == "sd") return TrendStat::SD;
    if (n == "cv") return TrendStat::CV;
    if (n == "min") return TrendStat::Min;
    if (n == "p5") return TrendStat::P5;
    if (n == "p25") return TrendStat::P25;
    if (n == "p75") return TrendStat::P75;
    if (n == "p95") return TrendStat::P95;
    if (n == "max") return TrendStat::Max;
    if (n == "range") return TrendStat::Range;
    if (n == "range_st") return TrendStat::RangeSemitones;
    if (n == "voiced") return TrendStat::Voiced;
    if (n == "count") return TrendStat::Count;
    return TrendStat::Median;
}

int Main::trendTrackFromName(const QString &name)
{
    const QString n = name.toLower();
    if (n == "f1") return 1;
    if (n == "f2") return 2;
    if (n == "f3") return 3;
    if (n == "f4") return 4;
    return 0;
}

bool Main::statValue(const PracticeRecord &r, int track, TrendStat stat, double *value)
{
    if (track < 0 || track > 4) return false;
    const TrackStats &s = track == 0 ? r.pitch : r.formants[track - 1];

    // The share and the number of values need only the frames. The other statistics need values.
    if (stat == TrendStat::Voiced) {
        if (s.frames <= 0) return false;
        *value = 100.0 * s.count / s.frames;
        return true;
    }
    if (stat == TrendStat::Count) {
        if (s.frames <= 0) return false;
        *value = s.count;
        return true;
    }
    if (s.count <= 0) return false;

    switch (stat) {
    case TrendStat::Median: *value = s.median; return true;
    case TrendStat::Mean: *value = s.mean; return true;
    case TrendStat::Min: *value = s.min; return true;
    case TrendStat::P5: *value = s.p5; return true;
    case TrendStat::P25: *value = s.p25; return true;
    case TrendStat::P75: *value = s.p75; return true;
    case TrendStat::P95: *value = s.p95; return true;
    case TrendStat::Max: *value = s.max; return true;
    case TrendStat::SD:
        if (s.count < 2) return false;
        *value = s.sd;
        return true;
    case TrendStat::CV:
        if (s.count < 2 || !(s.mean > 0.0)) return false;
        *value = 100.0 * s.sd / s.mean;
        return true;
    case TrendStat::Range:
        if (s.count < 5 || !(s.p95 >= s.p5)) return false;
        *value = s.p95 - s.p5;
        return true;
    case TrendStat::RangeSemitones:
        if (s.count < 5 || !(s.p5 > 0.0) || !(s.p95 >= s.p5)) return false;
        *value = semitones(s.p5, s.p95);
        return true;
    case TrendStat::Voiced:
    case TrendStat::Count:
        break;
    }
    return false;
}

bool Main::measureValue(const PracticeRecord &r, TrendMeasure measure, double *value)
{
    switch (measure) {
    case TrendMeasure::Pitch: return statValue(r, 0, TrendStat::Median, value);
    case TrendMeasure::F1: return statValue(r, 1, TrendStat::Median, value);
    case TrendMeasure::F2: return statValue(r, 2, TrendStat::Median, value);
    case TrendMeasure::F3: return statValue(r, 3, TrendStat::Median, value);
    case TrendMeasure::F4: return statValue(r, 4, TrendStat::Median, value);
    case TrendMeasure::Range: return statValue(r, 0, TrendStat::RangeSemitones, value);
    case TrendMeasure::Variation: return statValue(r, 0, TrendStat::CV, value);
    }
    return false;
}

QString Main::periodKey(const QDate &date, TrendPeriod period)
{
    switch (period) {
    case TrendPeriod::Day: return date.toString("yyyy-MM-dd");
    case TrendPeriod::Week: {
        int year = 0;
        const int week = date.weekNumber(&year);
        return QString("%1-W%2").arg(year).arg(week, 2, 10, QChar('0'));
    }
    case TrendPeriod::Month: return date.toString("yyyy-MM");
    case TrendPeriod::Year: return date.toString("yyyy");
    }
    return QString();
}

namespace {

QDate periodStart(const QDate &date, TrendPeriod period)
{
    switch (period) {
    case TrendPeriod::Day: return date;
    case TrendPeriod::Week: return date.addDays(-(date.dayOfWeek() - 1));
    case TrendPeriod::Month: return QDate(date.year(), date.month(), 1);
    case TrendPeriod::Year: return QDate(date.year(), 1, 1);
    }
    return date;
}

double medianOf(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return n % 2 == 1 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

}

std::vector<TrendPoint> Main::buildTrend(const QList<PracticeRecord> &records, TrendMeasure measure,
                                         TrendPeriod period, const QString &tagFilter)
{
    switch (measure) {
    case TrendMeasure::Pitch: return buildTrend(records, 0, TrendStat::Median, period, tagFilter);
    case TrendMeasure::F1: return buildTrend(records, 1, TrendStat::Median, period, tagFilter);
    case TrendMeasure::F2: return buildTrend(records, 2, TrendStat::Median, period, tagFilter);
    case TrendMeasure::F3: return buildTrend(records, 3, TrendStat::Median, period, tagFilter);
    case TrendMeasure::F4: return buildTrend(records, 4, TrendStat::Median, period, tagFilter);
    case TrendMeasure::Range: return buildTrend(records, 0, TrendStat::RangeSemitones, period, tagFilter);
    case TrendMeasure::Variation: return buildTrend(records, 0, TrendStat::CV, period, tagFilter);
    }
    return {};
}

std::vector<TrendPoint> Main::buildTrend(const QList<PracticeRecord> &records, int track, TrendStat stat,
                                         TrendPeriod period, const QString &tagFilter)
{
    struct Group {
        QDate start;
        std::vector<double> values;
        double minutes = 0.0;
    };
    std::map<QString, Group> groups;

    for (const PracticeRecord &r : records) {
        if (!tagFilter.isEmpty() && r.tag.compare(tagFilter, Qt::CaseInsensitive) != 0) continue;
        double value = 0.0;
        if (!statValue(r, track, stat, &value)) continue;

        const QDate date = r.start.date();
        Group &g = groups[periodKey(date, period)];
        g.start = periodStart(date, period);
        g.values.push_back(value);
        g.minutes += r.duration / 60.0;
    }

    std::vector<TrendPoint> out;
    for (const auto &entry : groups) {
        const Group &g = entry.second;
        TrendPoint p;
        p.key = entry.first;
        p.start = g.start;
        p.median = medianOf(g.values);
        p.min = *std::min_element(g.values.begin(), g.values.end());
        p.max = *std::max_element(g.values.begin(), g.values.end());
        p.minutes = g.minutes;
        p.sessions = (int) g.values.size();
        out.push_back(p);
    }
    return out;
}

const PracticeRecord *Main::previousSameTag(const QList<PracticeRecord> &records, const PracticeRecord &current)
{
    const PracticeRecord *best = nullptr;
    for (const PracticeRecord &r : records) {
        if (r.id == current.id) continue;
        if (r.tag.compare(current.tag, Qt::CaseInsensitive) != 0) continue;
        if (!(r.start < current.start)) continue;
        if (best == nullptr || best->start < r.start) best = &r;
    }
    return best;
}

QStringList Main::compareSessions(const PracticeRecord &current, const PracticeRecord *previous)
{
    QStringList lines;
    if (previous == nullptr) {
        lines << QString("This is the first session with the task \"%1\".").arg(current.tag);
        return lines;
    }

    const QString since = previous->start.toString("yyyy-MM-dd");
    auto change = [&](const QString &name, double now, double before, const QString &unit) {
        const double delta = now - before;
        QString direction = "no change";
        if (std::fabs(delta) >= 0.05) direction = QString("%1 %2 %3").arg(delta > 0 ? "up" : "down").arg(std::fabs(delta), 0, 'f', 1).arg(unit);
        return QString("%1: %2 %3, %4 since %5.").arg(name).arg(now, 0, 'f', 1).arg(unit).arg(direction, since);
    };

    double a = 0.0;
    double b = 0.0;
    if (measureValue(current, TrendMeasure::Pitch, &a) && measureValue(*previous, TrendMeasure::Pitch, &b)) {
        lines << change("Median pitch", a, b, "Hz");
        lines << QString("The change is %1 semitones.").arg(semitones(b, a), 0, 'f', 2);
    }
    for (int i = 0; i < 3; ++i) {
        const TrendMeasure m = (TrendMeasure) ((int) TrendMeasure::F1 + i);
        if (measureValue(current, m, &a) && measureValue(*previous, m, &b)) {
            lines << change(QString("Median F%1").arg(i + 1), a, b, "Hz");
        }
    }
    if (measureValue(current, TrendMeasure::Range, &a) && measureValue(*previous, TrendMeasure::Range, &b)) {
        lines << change("Pitch range", a, b, "semitones");
    }
    if (lines.isEmpty()) lines << "The two sessions have no value in common.";
    return lines;
}

QString Main::historyCsv(const QList<PracticeRecord> &records)
{
    QString out = "id,start,duration_s,task,note,pitch_median_hz,pitch_mean_hz,pitch_sd_hz,pitch_p5_hz,pitch_p95_hz,"
                  "pitch_range_semitones,pitch_variation_percent,f1_median_hz,f2_median_hz,f3_median_hz,f4_median_hz,marks\n";
    for (const PracticeRecord &r : records) {
        double range = 0.0;
        double variation = 0.0;
        measureValue(r, TrendMeasure::Range, &range);
        measureValue(r, TrendMeasure::Variation, &variation);
        out += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17\n")
            .arg(r.id, r.start.toString(Qt::ISODate), QString::number(r.duration, 'f', 1), csvField(r.tag), csvField(r.note))
            .arg(r.pitch.median, 0, 'f', 1).arg(r.pitch.mean, 0, 'f', 1).arg(r.pitch.sd, 0, 'f', 1)
            .arg(r.pitch.p5, 0, 'f', 1).arg(r.pitch.p95, 0, 'f', 1)
            .arg(range, 0, 'f', 2).arg(variation, 0, 'f', 1)
            .arg(r.formants[0].median, 0, 'f', 0).arg(r.formants[1].median, 0, 'f', 0)
            .arg(r.formants[2].median, 0, 'f', 0).arg(r.formants[3].median, 0, 'f', 0)
            .arg(r.marks.size());
    }
    return out;
}

double Main::practiceMinimumSeconds(const QVariant &option)
{
    bool ok = false;
    const double value = option.toDouble(&ok);
    if (!ok || std::isnan(value)) return 1.0;
    return std::clamp(value, 1.0, 3600.0);
}

bool Main::practiceTooShort(double lengthSeconds, double minimumSeconds)
{
    return lengthSeconds < minimumSeconds;
}

std::vector<TimelinePart> Main::timelineParts(double t0, double t1)
{
    std::vector<TimelinePart> parts;
    if (!(t1 > t0)) return parts;
    const double step = std::max(5.0, std::ceil((t1 - t0) / 12.0));
    for (double a = t0; a < t1; a += step) {
        parts.push_back({ a, std::min(a + step, t1) });
    }
    return parts;
}

QStringList Main::practiceBundleFiles(bool keepSound)
{
    QStringList files = { "session.json", "report.md", "stats.csv", "marks.csv" };
    if (keepSound) files << "audio.wav";
    return files;
}

QString Main::marksCsv(const QList<PracticeMark> &marks)
{
    QString out = "time_s,note,pitch_hz,f1_hz,f2_hz,f3_hz,f4_hz\n";
    for (const PracticeMark &m : marks) {
        out += QString("%1,%2,%3,%4,%5,%6,%7\n").arg(m.time, 0, 'f', 1).arg(csvField(m.note)).arg(m.pitch, 0, 'f', 1)
            .arg(m.formants[0], 0, 'f', 0).arg(m.formants[1], 0, 'f', 0).arg(m.formants[2], 0, 'f', 0).arg(m.formants[3], 0, 'f', 0);
    }
    return out;
}

bool Main::writeBundleText(const QString &folder, const PracticeRecord &record, const QString &reportText)
{
    bool ok = true;
    auto write = [&](const QString &name, const QString &text) {
        QFile file(QDir(folder).filePath(name));
        const QByteArray bytes = text.toUtf8();
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(bytes) != bytes.size()) ok = false;
    };
    write("report.md", reportText);
    write("stats.csv", historyCsv({ record }));
    write("marks.csv", marksCsv(record.marks));
    return ok;
}

PracticeExport Main::exportPracticeLibrary(const PracticeLibrary &library, const QString &target, const QDateTime &now)
{
    PracticeExport result;
    const QList<PracticeRecord> records = library.list();
    result.total = records.size();
    result.folder = QDir(target).filePath("InFormant-practice-" + now.toString("yyyyMMdd-HHmmss"));
    if (!QDir().mkpath(result.folder)) {
        result.message = QString("The target folder is not writable: %1").arg(target);
        return result;
    }

    for (const PracticeRecord &r : records) {
        const QString source = library.sessionFolder(r);
        const QString dest = QDir(result.folder).filePath(QDir(library.root()).relativeFilePath(source));
        bool ok = QDir().mkpath(dest);
        for (const QString &name : QDir(source).entryList(QDir::Files)) {
            QFile::remove(QDir(dest).filePath(name));
            ok = QFile::copy(QDir(source).filePath(name), QDir(dest).filePath(name)) && ok;
        }
        if (ok) ++result.copied;
    }

    QFile history(QDir(result.folder).filePath("history.csv"));
    const QByteArray csv = historyCsv(records).toUtf8();
    const bool historyOk = history.open(QIODevice::WriteOnly | QIODevice::Truncate) && history.write(csv) == csv.size();

    result.ok = historyOk && result.copied == result.total;
    result.message = QString("Exported %1 of %2 sessions").arg(result.copied).arg(result.total);
    if (!historyOk) result.message += ". The history file failed";
    return result;
}

PracticeLibrary::PracticeLibrary(const QString &root)
    : mRoot(root)
{
}

QString PracticeLibrary::sessionFolder(const PracticeRecord &r) const
{
    const QDate d = r.start.date();
    return QDir(mRoot).filePath(d.toString("yyyy") + "/" + d.toString("MM") + "/" + d.toString("dd") + "/" + r.id);
}

bool PracticeLibrary::save(const PracticeRecord &r) const
{
    const QString folder = sessionFolder(r);
    if (!QDir().mkpath(folder)) return false;
    QFile file(QDir(folder).filePath("session.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    const QByteArray bytes = QJsonDocument(recordToJson(r)).toJson(QJsonDocument::Indented);
    return file.write(bytes) == bytes.size();
}

QList<PracticeRecord> PracticeLibrary::list() const
{
    QList<PracticeRecord> out;
    QDirIterator it(mRoot, QStringList() << "session.json", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFile file(it.next());
        if (!file.open(QIODevice::ReadOnly)) continue;
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) continue;
        PracticeRecord r = recordFromJson(doc.object());
        if (r.id.isEmpty() || !r.start.isValid()) continue;
        out.append(r);
    }
    std::sort(out.begin(), out.end(), [](const PracticeRecord &x, const PracticeRecord &y) { return x.start < y.start; });
    return out;
}
