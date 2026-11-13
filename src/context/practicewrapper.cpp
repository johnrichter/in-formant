#include "practicewrapper.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>
#include <QUrl>
#include <algorithm>
#include <cmath>

#include "config.h"
#include "datastore.h"
#include "recordstats.h"
#include "sessionwrapper.h"

using namespace Main;

namespace {

QString clockText(double seconds)
{
    const int s = (int) std::max(0.0, seconds);
    return QString("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QChar('0'));
}

// The median of a track in a time range, or 0 when the range has no value.
double medianIn(OptionalTimeTrack<double> &track, double t0, double t1)
{
    const Summary s = summarizeTrack(track, t0, t1);
    return s.empty() ? 0.0 : s.median();
}

}

PracticeWrapper::PracticeWrapper(Config *config, SessionWrapper *session)
    : mConfig(config),
      mSession(session),
      mTag(config->optionString("practiceTag"))
{
    if (!tags().contains(mTag)) mTag = tags().front();

    mTimer.setInterval(500);
    connect(&mTimer, &QTimer::timeout, this, &PracticeWrapper::tick);

    // A session that is still open at the end of the program is written to the folder first.
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        if (mActive) {
            finish(true);
        }
        else if (mPending) {
            // The question is open at the quit. The session stays, because the person cannot answer.
            mPending = false;
            commit(true);
        }
    });
}

PracticeWrapper::~PracticeWrapper()
{
    if (mSoundThread.joinable()) mSoundThread.join();
}

QStringList PracticeWrapper::tags() const
{
    return { "Free", "Vowels", "Reading", "Conversation" };
}

QString PracticeWrapper::folder() const
{
    const QString custom = mConfig->optionString("practiceFolder").trimmed();
    if (!custom.isEmpty()) return custom;
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).filePath("InFormant/Practice");
}

void PracticeWrapper::setTag(const QString &tag)
{
    if (tag == mTag || !tags().contains(tag)) return;
    mTag = tag;
    mConfig->setOption("practiceTag", tag);
    emit tagChanged();
}

void PracticeWrapper::start()
{
    if (mActive) return;
    if (mSession->fileMode()) {
        mSession->showStatus(QStringLiteral("Return to the live view before a practice session"));
        return;
    }
    if (mSession->recording()) {
        mSession->showStatus(QStringLiteral("Stop the recording before a practice session"));
        return;
    }
    if (mConfig->isPaused()) mConfig->setPaused(false);

    mT0 = mSession->dataStore()->session().duration();
    mStart = QDateTime::currentDateTime();
    mMarks.clear();
    mElapsed = QStringLiteral("0:00");
    mActive = true;
    mTimer.start();
    emit activeChanged();
    emit elapsedChanged();
    emit markCountChanged();
    mSession->showStatus(QStringLiteral("Practice started. Press M to mark."));
}

void PracticeWrapper::tick()
{
    if (!mActive) return;
    const QString text = clockText(mSession->dataStore()->session().duration() - mT0);
    if (text != mElapsed) {
        mElapsed = text;
        emit elapsedChanged();
    }
}

PracticeMark PracticeWrapper::readMark(double now, const QString &note) const
{
    DataStore &store = *mSession->dataStore();
    PracticeMark m;
    m.time = now - mT0;
    m.note = note.trimmed();

    store.beginRead();
    m.pitch = medianIn(store.getPitchTrack(), now - 0.5, now);
    for (int i = 0; i < std::min(store.getFormantTrackCount(), 4); ++i) {
        m.formants[i] = medianIn(store.getFormantTrack(i), now - 0.5, now);
    }
    store.endRead();
    return m;
}

void PracticeWrapper::mark(const QString &note)
{
    if (!mActive) return;
    mMarks.append(readMark(mSession->dataStore()->session().duration(), note));
    emit markCountChanged();
    const PracticeMark &m = mMarks.last();
    mSession->showStatus(m.pitch > 0 ? QString("Mark %1 at %2, pitch %3 Hz").arg(mMarks.size()).arg(clockText(m.time)).arg(m.pitch, 0, 'f', 0)
                                     : QString("Mark %1 at %2, no pitch").arg(mMarks.size()).arg(clockText(m.time)));
}

PracticeRecord PracticeWrapper::buildRecord(double t0, double t1, const QDateTime &start) const
{
    DataStore &store = *mSession->dataStore();

    PracticeRecord r;
    r.id = start.toString("yyyyMMdd-HHmmss");
    r.start = start;
    r.duration = t1 - t0;
    r.tag = mTag;
    r.marks = mMarks;

    store.beginRead();
    r.pitch = statsFrom(summarizeTrack(store.getPitchTrack(), t0, t1));
    const int formantCount = std::min(store.getFormantTrackCount(), 4);
    for (int i = 0; i < formantCount; ++i) r.formants[i] = statsFrom(summarizeTrack(store.getFormantTrack(i), t0, t1));

    // The median of each part of the session. The parts have equal length, and there are at most 12.
    const double step = std::max(5.0, std::ceil((t1 - t0) / 12.0));
    for (double a = t0; a < t1; a += step) {
        const double b = std::min(a + step, t1);
        PracticeTimelinePoint p;
        p.time = a - t0;
        p.pitch = medianIn(store.getPitchTrack(), a, b);
        for (int i = 0; i < formantCount; ++i) p.formants[i] = medianIn(store.getFormantTrack(i), a, b);
        r.timeline.append(p);
    }
    store.endRead();
    return r;
}

void PracticeWrapper::writeBundle(const PracticeRecord &record, const QString &sessionFolder, double t0, double t1, bool waitForSound)
{
    QStringList files = { "session.json", "report.md", "stats.csv", "marks.csv" };
    const bool keepSound = mConfig->optionBool("practiceKeepAudio");
    if (keepSound) files << "audio.wav";

    auto write = [&](const QString &name, const QString &text) {
        QFile file(QDir(sessionFolder).filePath(name));
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) file.write(text.toUtf8());
    };

    write("report.md", buildRecordingReport(*mConfig, *mSession->dataStore(), t0, t1, record.start, files));
    write("stats.csv", historyCsv({ record }));

    QString marks = "time_s,note,pitch_hz,f1_hz,f2_hz,f3_hz,f4_hz\n";
    for (const PracticeMark &m : record.marks) {
        QString note = m.note;
        note.replace('"', "\"\"");
        marks += QString("%1,\"%2\",%3,%4,%5,%6,%7\n").arg(m.time, 0, 'f', 1).arg(note).arg(m.pitch, 0, 'f', 1)
            .arg(m.formants[0], 0, 'f', 0).arg(m.formants[1], 0, 'f', 0).arg(m.formants[2], 0, 'f', 0).arg(m.formants[3], 0, 'f', 0);
    }
    write("marks.csv", marks);

    if (keepSound) {
        if (mSoundThread.joinable()) mSoundThread.join();
        const QString path = QDir(sessionFolder).filePath("audio.wav");
        Session *session = &mSession->dataStore()->session();
        if (waitForSound) {
            session->exportWav(path, t0, t1);
        }
        else {
            mSoundThread = std::thread([session, path, t0, t1]() { session->exportWav(path, t0, t1); });
        }
    }
}

void PracticeWrapper::stop()
{
    if (mActive) finish(false);
}

void PracticeWrapper::finish(bool quitting)
{
    mTimer.stop();
    mActive = false;
    emit activeChanged();

    const double t1 = mSession->dataStore()->session().duration();
    const double minimum = std::max(1.0, mConfig->optionDouble("practiceMinSeconds"));
    if (t1 - mT0 < minimum) {
        mMarks.clear();
        emit markCountChanged();
        mSession->showStatus(QString("The practice was shorter than %1 seconds. Nothing is saved.").arg(minimum, 0, 'f', 0));
        return;
    }

    mPendingT1 = t1;
    mPending = true;

    // A quit saves the session without a question, because the person cannot answer.
    if (quitting) {
        mPending = false;
        commit(true);
        return;
    }

    // The person decides. The text shows the length, the marks, and the comparison with the last session of the task.
    const PracticeRecord record = buildRecord(mT0, t1, mStart);
    PracticeLibrary library(folder());
    QStringList lines;
    lines << QString("Task: %1. Length: %2. Marks: %3.").arg(record.tag, clockText(record.duration)).arg(record.marks.size());
    lines << compareSessions(record, previousSameTag(library.list(), record));
    emit confirm(lines.join('\n'));
}

void PracticeWrapper::keep()
{
    if (!mPending) return;
    mPending = false;
    commit(false);
}

void PracticeWrapper::discard()
{
    if (!mPending) return;
    mPending = false;
    mMarks.clear();
    emit markCountChanged();
    mSession->showStatus(QStringLiteral("The practice is discarded. Nothing is saved."));
}

void PracticeWrapper::commit(bool waitForSound)
{
    const double t1 = mPendingT1;
    PracticeRecord record = buildRecord(mT0, t1, mStart);

    PracticeLibrary library(folder());
    const QList<PracticeRecord> before = library.list();
    const QString sessionFolder = library.sessionFolder(record);
    if (!library.save(record)) {
        mSummary = QString("The folder of the sessions is not writable: %1").arg(folder());
        emit summaryChanged();
        mSession->showStatus(QStringLiteral("The practice could not be saved"));
        return;
    }
    writeBundle(record, sessionFolder, mT0, t1, waitForSound);

    QStringList lines;
    lines << QString("Task: %1. Length: %2. Marks: %3.").arg(record.tag, clockText(record.duration)).arg(record.marks.size());
    lines << compareSessions(record, previousSameTag(before, record));
    lines << QString("Folder: %1").arg(sessionFolder);
    mSummary = lines.join('\n');

    mMarks.clear();
    emit markCountChanged();
    emit summaryChanged();
    emit folderChanged();
    mSession->showStatus(QStringLiteral("Practice saved"));
    emit saved(mSummary);
}

void PracticeWrapper::openFolder()
{
    QDir().mkpath(folder());
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder()));
}

void PracticeWrapper::exportAll()
{
    const QString target = QFileDialog::getExistingDirectory(nullptr, tr("Export all practice sessions"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (target.isEmpty()) return;

    PracticeLibrary library(folder());
    const QList<PracticeRecord> records = library.list();
    const QString out = QDir(target).filePath("InFormant-practice-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
    QDir().mkpath(out);

    int copied = 0;
    for (const PracticeRecord &r : records) {
        const QString source = library.sessionFolder(r);
        const QString dest = QDir(out).filePath(QDir(library.root()).relativeFilePath(source));
        QDir().mkpath(dest);
        const QStringList names = QDir(source).entryList(QDir::Files);
        bool ok = true;
        for (const QString &name : names) {
            QFile::remove(QDir(dest).filePath(name));
            ok = QFile::copy(QDir(source).filePath(name), QDir(dest).filePath(name)) && ok;
        }
        if (ok) ++copied;
    }

    QFile history(QDir(out).filePath("history.csv"));
    if (history.open(QIODevice::WriteOnly | QIODevice::Truncate)) history.write(historyCsv(records).toUtf8());

    mSession->showStatus(QString("Exported %1 of %2 sessions").arg(copied).arg(records.size()));
}

QVariantList PracticeWrapper::trend(const QString &track, const QString &stat, const QString &period, const QString &tag)
{
    PracticeLibrary library(folder());
    const auto points = buildTrend(library.list(), trendTrackFromName(track), trendStatFromName(stat),
                                   trendPeriodFromName(period), tag);

    QVariantList out;
    for (const TrendPoint &p : points) {
        QVariantMap m;
        m["key"] = p.key;
        m["start"] = p.start.toString("yyyy-MM-dd");
        m["median"] = p.median;
        m["min"] = p.min;
        m["max"] = p.max;
        m["minutes"] = p.minutes;
        m["sessions"] = p.sessions;
        out << m;
    }
    return out;
}

QVariantList PracticeWrapper::sessions()
{
    PracticeLibrary library(folder());
    QList<PracticeRecord> records = library.list();

    QVariantList out;
    for (int i = records.size() - 1; i >= 0; --i) {
        const PracticeRecord &r = records[i];
        QVariantMap m;
        m["id"] = r.id;
        m["start"] = r.start.toString("yyyy-MM-dd HH:mm");
        m["minutes"] = r.duration / 60.0;
        m["tag"] = r.tag;
        m["pitch"] = r.pitch.count > 0 ? r.pitch.median : 0.0;
        for (int f = 0; f < 3; ++f) m[QString("f%1").arg(f + 1)] = r.formants[f].count > 0 ? r.formants[f].median : 0.0;
        m["marks"] = r.marks.size();
        out << m;
    }
    return out;
}

QStringList PracticeWrapper::libraryTags()
{
    PracticeLibrary library(folder());
    QStringList out;
    for (const PracticeRecord &r : library.list()) {
        if (!r.tag.isEmpty() && !out.contains(r.tag)) out << r.tag;
    }
    out.sort();
    return out;
}
