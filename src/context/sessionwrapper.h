#ifndef MAIN_CONTEXT_SESSION_WRAPPER_H
#define MAIN_CONTEXT_SESSION_WRAPPER_H

#include <atomic>
#include <thread>

#include <QObject>
#include <QString>
#include <QTimer>

#include "config.h"
#include "datastore.h"
#include "fileanalyzer.h"
#include "recorder.h"
#include "sessionplayer.h"

namespace Main {

    // The controls of the session for the interface: the playback of a window of the sound, the
    // return to the live view, and the export of the sound as a WAV file.
    class SessionWrapper : public QObject {
        Q_OBJECT
        Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
        Q_PROPERTY(bool exporting READ exporting NOTIFY exportingChanged)
        Q_PROPERTY(bool fileMode READ fileMode NOTIFY fileModeChanged)
        Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
        Q_PROPERTY(QString recordTime READ recordTime NOTIFY recordingChanged)
        Q_PROPERTY(QString status READ status NOTIFY statusChanged)

    public:
        // The analysis sample rate is the rate of the live analysis. The reset function prepares the live
        // analysis for a new capture that starts at time zero.
        SessionWrapper(Config *config, DataStore *dataStore, SessionPlayer *player, int analysisSampleRate,
                       std::function<void()> resetLive);
        ~SessionWrapper() override;

        DataStore *dataStore() const { return mDataStore; }

        bool playing() const { return mPlaying; }
        bool exporting() const { return mExporting.load(); }
        bool fileMode() const { return mConfig->getSessionMode() == 1; }
        bool recording() const { return mRecorder.recording(); }
        QString recordTime() const;

        // The window with the main graph, for the pictures of the video.
        void setMainWindow(QQuickWindow *window) { mRecorder.setMainWindow(window); }
        QString status() const { return mStatus; }

        // The window of the view in seconds of the session.
        Q_INVOKABLE double windowStart() const;
        Q_INVOKABLE double windowEnd() const;

        // Play the sound of the window of the view. The capture pauses during the playback.
        Q_INVOKABLE void playWindow();
        Q_INVOKABLE void stopPlayback();

        // Move the end of the view to a time in seconds. A time at the end of the sound shows the
        // newest sound again while the capture runs. A move stops a playback.
        Q_INVOKABLE void moveViewEnd(double end, double edge = 0.0);

        // Move the view by steps of a tenth of its width. A positive number moves to a later time.
        Q_INVOKABLE void scrollView(double steps);

        // Start or stop the recording of the live capture to a video, a sound file and a report.
        Q_INVOKABLE void toggleRecording();

        // Start the recording at a path without extension, or stop it.
        Q_INVOKABLE void recordTo(const QString &base);

        // Ask for an audio file, load the whole file, and analyze it. The file replaces the live sound.
        Q_INVOKABLE void openFile();

        // Load and analyze the file at this path, without a dialog.
        Q_INVOKABLE void loadFile(const QString &path);

        // Show the newest sound again, and continue the capture. After a file, this deletes the
        // file and starts a new capture.
        Q_INVOKABLE void goLive();

        // Ask for a file name, and write the sound of the window, or the whole session, as a WAV file.
        Q_INVOKABLE void exportWindow();
        Q_INVOKABLE void exportAll();

    signals:
        void playingChanged();
        void exportingChanged();
        void statusChanged();
        void exportFinished(const QString &path, bool ok);
        void fileModeChanged();
        void recordingChanged();

    private:
        void tick();
        void finishPlayback();
        void setStatus(const QString &status);
        void exportRange(double t0, double t1, const QString &suffix);
        double clampViewEnd(double end) const;
        void stopFileWork();
        void clearSession();
        void onLoaded(bool ok, const QString &error, const QString &name);
        void onAnalyzed(bool completed, const QString &name);

        Config *mConfig;
        DataStore *mDataStore;
        SessionPlayer *mPlayer;

        Recorder mRecorder;
        FileAnalyzer mAnalyzer;
        std::function<void()> mResetLive;
        std::thread mLoadThread;
        std::atomic<bool> mLoadCancel;
        QTimer mProgressTimer;

        QTimer mTimer;
        bool mPlaying;
        bool mPausedByPlayback;
        QString mStatus;

        std::atomic<bool> mExporting;
        std::thread mExportThread;
    };

}

#endif // MAIN_CONTEXT_SESSION_WRAPPER_H
