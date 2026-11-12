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

bool Main::measureValue(const PracticeRecord &r, TrendMeasure measure, double *value)
{
    switch (measure) {
    case TrendMeasure::Pitch:
        if (r.pitch.count <= 0) return false;
        *value = r.pitch.median;
        return true;
    case TrendMeasure::F1:
    case TrendMeasure::F2:
    case TrendMeasure::F3:
    case TrendMeasure::F4: {
        const int i = (int) measure - (int) TrendMeasure::F1;
        if (r.formants[i].count <= 0) return false;
        *value = r.formants[i].median;
        return true;
    }
    case TrendMeasure::Range:
        if (r.pitch.count < 5 || !(r.pitch.p5 > 0.0) || !(r.pitch.p95 >= r.pitch.p5)) return false;
        *value = semitones(r.pitch.p5, r.pitch.p95);
        return true;
    case TrendMeasure::Variation:
        if (r.pitch.count < 2 || !(r.pitch.mean > 0.0)) return false;
        *value = 100.0 * r.pitch.sd / r.pitch.mean;
        return true;
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
    struct Group {
        QDate start;
        std::vector<double> values;
        double minutes = 0.0;
    };
    std::map<QString, Group> groups;

    for (const PracticeRecord &r : records) {
        if (!tagFilter.isEmpty() && r.tag.compare(tagFilter, Qt::CaseInsensitive) != 0) continue;
        double value = 0.0;
        if (!measureValue(r, measure, &value)) continue;

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
