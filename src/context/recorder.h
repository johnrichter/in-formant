#ifndef MAIN_CONTEXT_RECORDER_H
#define MAIN_CONTEXT_RECORDER_H

#include <memory>

#include <QDateTime>
#include <QImage>
#include <QObject>
#include <QQuickWindow>
#include <QString>
#include <QTimer>

#include "config.h"
#include "datastore.h"
#include "videowriter.h"

namespace Main {

    // Records the live capture to three files: a video of the graph and of the other tools, the sound
    // as a WAV file, and a Markdown report with statistics. The video shows the main graph, and the
    // other windows in a column at the right or in a row below. The windows are scaled so that the
    // picture has no empty space.
    class Recorder : public QObject {
        Q_OBJECT

    public:
        Recorder(Config *config, DataStore *dataStore, QObject *parent = nullptr);
        ~Recorder() override;

        // The window with the main graph. The recorder takes the pictures of it.
        void setMainWindow(QQuickWindow *window) { mWindow = window; }

        bool recording() const { return mRecording; }

        // The length of the recording in seconds.
        double elapsed() const;

        // Ask for a file name, and start. The call returns false when the user cancels or the start fails.
        bool start();

        // Start without a question. The base is the path of the files without the extension.
        bool begin(const QString &base);

        // Stop, and write the files.
        void stop();

    signals:
        void message(const QString &text);
        void recordingChanged();

    private:
        void tick();
        bool writeAudio(double until);
        QImage composeFrame();
        void restoreView();

        Config *mConfig;
        DataStore *mDataStore;
        QQuickWindow *mWindow;
        QTimer mTimer;

        bool mRecording;
        QString mBase;
        QDateTime mStarted;
        double mStart;
        long long mAudioSamples;
        double mLastTime;

        std::unique_ptr<VideoWriter> mVideo;
        QSize mSize;
        int mFrames;
        double mComposeMs;

        double mSavedSpan;
        bool mSavedLive;
    };

}

#endif // MAIN_CONTEXT_RECORDER_H
