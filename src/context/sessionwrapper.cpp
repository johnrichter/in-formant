#include "sessionwrapper.h"

#include "audiofile.h"

#include <algorithm>

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMetaObject>
#include <QStandardPaths>

using namespace Main;

SessionWrapper::SessionWrapper(Config *config, DataStore *dataStore, SessionPlayer *player, int analysisSampleRate,
                               std::function<void()> resetLive)
    : mConfig(config),
      mDataStore(dataStore),
      mPlayer(player),
      mRecorder(config, dataStore),
      mAnalyzer(config, dataStore, analysisSampleRate),
      mResetLive(std::move(resetLive)),
      mLoadCancel(false),
      mPlaying(false),
      mPausedByPlayback(false),
      mExporting(false)
{
    connect(&mRecorder, &Recorder::message, this, [this](const QString &text) { setStatus(text); });
    connect(&mRecorder, &Recorder::recordingChanged, this, &SessionWrapper::recordingChanged);

    mTimer.setInterval(33);
    connect(&mTimer, &QTimer::timeout, this, &SessionWrapper::tick);
}

SessionWrapper::~SessionWrapper()
{
    stopFileWork();
    if (mExportThread.joinable()) mExportThread.join();
}

void SessionWrapper::stopFileWork()
{
    mLoadCancel = true;
    if (mLoadThread.joinable()) mLoadThread.join();
    mAnalyzer.cancel();
    mProgressTimer.stop();
}

void SessionWrapper::clearSession()
{
    mDataStore->beginWrite();
    mDataStore->clearTracks();
    mDataStore->endWrite();
    mDataStore->session().clear();
    mConfig->clearMoment();
}

void SessionWrapper::openFile()
{
    if (mRecorder.recording()) return;

    const QString folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getOpenFileName(nullptr, tr("Open sound"), folder,
        tr("Audio files (*.wav *.mp3 *.m4a *.aac *.flac *.aif *.aiff *.caf *.ogg);;All files (*)"));
    if (path.isEmpty()) return;
    loadFile(path);
}

void SessionWrapper::loadFile(const QString &path)
{
    if (mRecorder.recording()) return;

    const QString name = QFileInfo(path).fileName();

    if (mPlaying) stopPlayback();
    stopFileWork();

    // The file replaces the live sound. The pipeline ignores the input device in this mode.
    mConfig->setSessionMode(1);
    if (!mConfig->isPaused()) mConfig->setPaused(true);
    clearSession();
    mConfig->setViewLive(false);
    mConfig->setViewEnd(0.0);
    emit fileModeChanged();
    setStatus("Loading " + name);

    mLoadCancel = false;
    mLoadThread = std::thread([this, path, name]() {
        QString error;
        Session &session = mDataStore->session();
        const bool ok = decodeAudioFile(path, [&](const float *samples, size_t count, double fs) {
            if (mLoadCancel.load()) return false;
            if (fs == Session::sampleRate) {
                session.appendStored(samples, count);
            }
            else {
                const std::vector<double> converted(samples, samples + count);
                session.append(converted.data(), (int) count, fs);
            }
            return true;
        }, error);

        if (mLoadCancel.load()) return;
        QMetaObject::invokeMethod(this, [this, ok, error, name]() { onLoaded(ok, error, name); }, Qt::QueuedConnection);
    });
}

void SessionWrapper::onLoaded(bool ok, const QString &error, const QString &name)
{
    if (mLoadThread.joinable()) mLoadThread.join();

    if (!ok) {
        setStatus(error.isEmpty() ? QStringLiteral("Cannot read the file") : error);
        return;
    }

    setStatus("Analyzing 0%");
    mAnalyzer.start([this, name](bool completed) {
        QMetaObject::invokeMethod(this, [this, name, completed]() { onAnalyzed(completed, name); }, Qt::QueuedConnection);
    });

    disconnect(&mProgressTimer, nullptr, nullptr, nullptr);
    mProgressTimer.setInterval(250);
    connect(&mProgressTimer, &QTimer::timeout, this, [this]() {
        if (mAnalyzer.running()) setStatus(QStringLiteral("Analyzing %1%").arg((int) (mAnalyzer.progress() * 100.0)));
    });
    mProgressTimer.start();
}

void SessionWrapper::onAnalyzed(bool completed, const QString &name)
{
    mProgressTimer.stop();
    if (completed) setStatus("Loaded " + name);
}

double SessionWrapper::duration() const
{
    return mDataStore->session().duration();
}

double SessionWrapper::windowStart() const
{
    return windowEnd() - mConfig->getViewTimeSpan();
}

double SessionWrapper::clampViewEnd(double end) const
{
    const double length = mDataStore->session().duration();
    const double span = mConfig->getViewTimeSpan();

    // The view of a file starts at time zero, and it can show empty time after a short file.
    if (mConfig->getSessionMode() == 1) return std::clamp(end, span, std::max(length, span));
    return std::clamp(end, std::min(span, length), length);
}

double SessionWrapper::windowEnd() const
{
    if (mConfig->getSessionMode() == 0 && mConfig->getViewLive()) return mDataStore->session().duration();
    return clampViewEnd(mConfig->getViewEnd());
}

QString SessionWrapper::recordTime() const
{
    const int s = (int) mRecorder.elapsed();
    return QString("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QChar('0'));
}

void SessionWrapper::toggleRecording()
{
    if (mRecorder.recording()) {
        mRecorder.stop();
        return;
    }

    // A recording needs the live capture.
    if (mConfig->getSessionMode() != 0 || mPlaying) {
        setStatus(QStringLiteral("Return to the live view before a recording"));
        return;
    }
    if (mConfig->isPaused()) mConfig->setPaused(false);
    mRecorder.start();
}

void SessionWrapper::recordTo(const QString &base)
{
    if (mRecorder.recording() || mConfig->getSessionMode() != 0 || mPlaying) return;
    if (mConfig->isPaused()) mConfig->setPaused(false);
    mRecorder.begin(base);
}

void SessionWrapper::moveViewEnd(double end, double edge)
{
    // The view of a recording stays on the newest sound.
    if (mRecorder.recording()) return;
    if (mPlaying) stopPlayback();

    const double length = std::max(mDataStore->session().duration(), 1.0);

    if (mConfig->getSessionMode() == 0 && end >= length - edge) {
        mConfig->setViewLive(true);
        return;
    }

    mConfig->setViewEnd(clampViewEnd(end));
    mConfig->setViewLive(false);
}

void SessionWrapper::scrollView(double steps)
{
    const double span = mConfig->getViewTimeSpan();
    moveViewEnd(windowEnd() + steps * 0.1 * span, 0.0);
}

void SessionWrapper::playWindow()
{
    if (mRecorder.recording()) return;

    const double end = windowEnd();
    const double start = std::max(0.0, end - mConfig->getViewTimeSpan());
    if (!(end > start)) return;

    // The microphone would record the playback, so the capture pauses until the playback ends.
    if (mConfig->getSessionMode() == 0 && !mConfig->isPaused()) {
        mConfig->setPaused(true);
        mPausedByPlayback = true;
    }

    mConfig->setViewEnd(end);
    mConfig->setViewLive(false);
    mPlayer->start(start, end);
    mPlaying = true;
    mTimer.start();
    emit playingChanged();
    setStatus(QStringLiteral("Playing"));
}

void SessionWrapper::stopPlayback()
{
    mPlayer->stop();
    finishPlayback();
}

void SessionWrapper::goLive()
{
    mPlayer->stop();
    finishPlayback();

    // The sound of a loaded file goes away, and a new capture starts at time zero.
    if (mConfig->getSessionMode() == 1) {
        stopFileWork();
        clearSession();
        mConfig->setSessionMode(0);
        mResetLive();
        emit fileModeChanged();
        setStatus(QString());
    }

    mConfig->setViewLive(true);
    if (mConfig->isPaused()) mConfig->setPaused(false);
}

void SessionWrapper::finishPlayback()
{
    if (mPausedByPlayback) {
        mPausedByPlayback = false;
        mConfig->setPaused(false);
    }
    mTimer.stop();
    if (mPlaying) {
        mPlaying = false;
        emit playingChanged();
        setStatus(QString());
    }
}

void SessionWrapper::tick()
{
    if (!mPlaying) return;

    if (!mPlayer->active()) {
        finishPlayback();
        return;
    }

    // The view follows the playback, and the sound that plays is close to its right edge.
    const double span = mConfig->getViewTimeSpan();
    mConfig->setViewEnd(mPlayer->position() + 0.1 * span);
}

void SessionWrapper::setStatus(const QString &status)
{
    if (status == mStatus) return;
    mStatus = status;
    emit statusChanged();
}

void SessionWrapper::exportWindow()
{
    exportRange(windowStart(), windowEnd(), QStringLiteral("window"));
}

void SessionWrapper::exportAll()
{
    exportRange(0.0, mDataStore->session().duration(), QStringLiteral("session"));
}

void SessionWrapper::exportRange(double t0, double t1, const QString &suffix)
{
    if (mExporting.load()) return;
    if (!(std::min(t1, mDataStore->session().duration()) > std::max(t0, 0.0))) {
        setStatus(QStringLiteral("No sound to export"));
        return;
    }

    const QString folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString name = "informant-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + "-" + suffix + ".wav";
    QString path = QFileDialog::getSaveFileName(nullptr, tr("Export sound"), QDir(folder).filePath(name), tr("WAV audio (*.wav)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(".wav", Qt::CaseInsensitive)) path += ".wav";

    if (mExportThread.joinable()) mExportThread.join();
    mExporting = true;
    emit exportingChanged();
    setStatus(QStringLiteral("Exporting"));

    mExportThread = std::thread([this, path, t0, t1]() {
        const bool ok = mDataStore->session().exportWav(path, t0, t1);
        QMetaObject::invokeMethod(this, [this, path, ok]() {
            mExporting = false;
            emit exportingChanged();
            setStatus(ok ? QStringLiteral("Saved ") + QFileInfo(path).fileName() : QStringLiteral("Export failed"));
            emit exportFinished(path, ok);
        }, Qt::QueuedConnection);
    });
}
