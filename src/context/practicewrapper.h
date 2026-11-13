#ifndef MAIN_CONTEXT_PRACTICE_WRAPPER_H
#define MAIN_CONTEXT_PRACTICE_WRAPPER_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <thread>

#include "practicelib.h"

namespace Main {

    class Config;
    class SessionWrapper;

    // The practice mode for the interface. One click starts a session on the live capture. The person
    // sets marks while the session runs. A click on stop writes a folder with the statistics, a report,
    // the marks, and the sound.
    class PracticeWrapper : public QObject {
        Q_OBJECT
        Q_PROPERTY(bool active READ active NOTIFY activeChanged)
        Q_PROPERTY(QString elapsed READ elapsed NOTIFY elapsedChanged)
        Q_PROPERTY(int markCount READ markCount NOTIFY markCountChanged)
        Q_PROPERTY(QString tag READ tag WRITE setTag NOTIFY tagChanged)
        Q_PROPERTY(QStringList tags READ tags CONSTANT)
        Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)
        Q_PROPERTY(QString folder READ folder NOTIFY folderChanged)

    public:
        PracticeWrapper(Config *config, SessionWrapper *session);
        ~PracticeWrapper() override;

        bool active() const { return mActive; }
        QString elapsed() const { return mElapsed; }
        int markCount() const { return mMarks.size(); }
        QString tag() const { return mTag; }
        QStringList tags() const;
        QString summary() const { return mSummary; }
        QString folder() const;

        void setTag(const QString &tag);

        // Start a session on the live capture. The capture starts if it is paused.
        Q_INVOKABLE void start();

        // Stop the session. A session shorter than the minimum time (the option practiceMinSeconds) is
        // dropped. A longer session waits for the answer of the person: the signal confirm asks the
        // question, and the person answers with keep or discard.
        Q_INVOKABLE void stop();

        // Save the session that waits for an answer.
        Q_INVOKABLE void keep();

        // Drop the session that waits for an answer. Nothing is written.
        Q_INVOKABLE void discard();

        // Save a mark with the values of the last half second. The note can be empty.
        Q_INVOKABLE void mark(const QString &note);

        // Show the folder of the sessions in the file manager.
        Q_INVOKABLE void openFolder();

        // Ask for a folder, and copy all sessions and a history file there.
        Q_INVOKABLE void exportAll();

        // The data of the trends. The track is "pitch" or "f1" to "f4". The statistic is "median", "mean",
        // "sd", "cv", "min", "p5", "p25", "p75", "p95", "max", "range", "range_st", "voiced", or "count".
        // The period is "day", "week", "month", or "year". A tag of "" means all tasks. Each item has the
        // keys key, start, median, min, max, minutes, and sessions.
        Q_INVOKABLE QVariantList trend(const QString &track, const QString &stat, const QString &period, const QString &tag);

        // All sessions as items with the keys id, start, minutes, tag, pitch, f1, f2, f3, and marks. Newest first.
        Q_INVOKABLE QVariantList sessions();

        // The task names that occur in the library.
        Q_INVOKABLE QStringList libraryTags();

    signals:
        void activeChanged();
        void elapsedChanged();
        void markCountChanged();
        void tagChanged();
        void summaryChanged();
        void folderChanged();
        // Emitted when a session is saved. The text is the summary.
        void saved(const QString &text);
        // Emitted when a session has ended and waits for the answer keep or discard. The text describes the session.
        void confirm(const QString &text);

    private:
        void finish(bool quitting);
        void commit(bool waitForSound);
        void tick();
        PracticeRecord buildRecord(double t0, double t1, const QDateTime &start) const;
        PracticeMark readMark(double now, const QString &note) const;
        void writeBundle(const PracticeRecord &record, const QString &folder, double t0, double t1, bool waitForSound);

        Config *mConfig;
        SessionWrapper *mSession;
        QTimer mTimer;

        bool mActive = false;
        // A session that has ended and waits for the answer of the person.
        bool mPending = false;
        double mPendingT1 = 0.0;
        double mT0 = 0.0;
        QDateTime mStart;
        QString mElapsed = QStringLiteral("0:00");
        QString mTag;
        QString mSummary;
        QList<PracticeMark> mMarks;
        std::thread mSoundThread;
    };

}

#endif // MAIN_CONTEXT_PRACTICE_WRAPPER_H
