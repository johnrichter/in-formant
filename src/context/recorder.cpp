#include "recorder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPainter>
#include <QQuickItem>
#include <QStandardPaths>
#include <QTextStream>

#include "recordstats.h"

using namespace Main;

Recorder::Recorder(Config *config, DataStore *dataStore, QObject *parent)
    : QObject(parent),
      mConfig(config),
      mDataStore(dataStore),
      mWindow(nullptr),
      mRecording(false),
      mStart(0),
      mAudioSamples(0),
      mLastTime(0),
      mFrames(0),
      mComposeMs(0),
      mSavedSpan(15.0),
      mSavedLive(true)
{
    connect(&mTimer, &QTimer::timeout, this, &Recorder::tick);
}

Recorder::~Recorder()
{
    if (mRecording) stop();
}

double Recorder::elapsed() const
{
    return mRecording ? std::max(0.0, mDataStore->session().duration() - mStart) : 0.0;
}

QImage Recorder::composeFrame()
{
    if (mWindow == nullptr) return QImage();

    const QImage main = mWindow->grabWindow();
    if (main.isNull()) return QImage();

    // The graph is the area of the canvas, without the toolbar and the map.
    QRect crop(0, 0, main.width(), main.height());
    if (QQuickItem *canvas = mWindow->findChild<QQuickItem *>("IfCanvas")) {
        const qreal ratio = main.devicePixelRatio();
        const QRectF area = canvas->mapRectToScene(QRectF(0, 0, canvas->width(), canvas->height()));
        crop = QRect(qRound(area.x() * ratio), qRound(area.y() * ratio), qRound(area.width() * ratio), qRound(area.height() * ratio))
                   .intersected(main.rect());

        // The toolbar covers the top of the canvas.
        if (auto *header = mWindow->property("header").value<QQuickItem *>()) {
            const int below = qRound(header->mapRectToScene(QRectF(0, 0, header->width(), header->height())).bottom() * ratio);
            if (below > crop.top()) crop.setTop(below);
        }
    }
    const QImage graph = main.copy(crop).convertToFormat(QImage::Format_RGB32);

    // The oscilloscope and the synthesizer, when their windows are open.
    std::vector<QImage> others;
    if (mConfig->optionBool("recordWindows")) {
        for (QWindow *window : QGuiApplication::allWindows()) {
            auto *quick = qobject_cast<QQuickWindow *>(window);
            if (quick == nullptr || quick == mWindow || !quick->isVisible()) continue;

            const QString title = quick->title();
            if (!title.endsWith("Oscilloscope") && !title.endsWith("Synthesizer")) continue;

            const QImage picture = quick->grabWindow();
            if (!picture.isNull()) others.push_back(picture.convertToFormat(QImage::Format_RGB32));
        }
    }

    QImage frame = graph;
    if (!others.empty()) {
        // The other windows go in a column at the right of the graph, or in a row below it. In each
        // layout the windows fill the whole side, so the picture has no empty space. The layout with
        // the shape that is closer to 16:9 gives the larger picture on a screen.
        double heightsOverWidths = 0, widthsOverHeights = 0;
        for (const QImage &o : others) {
            heightsOverWidths += (double) o.height() / o.width();
            widthsOverHeights += (double) o.width() / o.height();
        }

        const int graphWidth = graph.width(), graphHeight = graph.height();
        const int column = std::max(2, (int) std::lround(graphHeight / heightsOverWidths));
        const int row = std::max(2, (int) std::lround(graphWidth / widthsOverHeights));

        const double target = 16.0 / 9.0;
        const double columnShape = (double) (graphWidth + column) / graphHeight;
        const double rowShape = (double) graphWidth / (graphHeight + row);
        const bool useColumn = std::abs(std::log(columnShape / target)) <= std::abs(std::log(rowShape / target));

        frame = useColumn ? QImage(graphWidth + column, graphHeight, QImage::Format_RGB32)
                          : QImage(graphWidth, graphHeight + row, QImage::Format_RGB32);
        frame.fill(Qt::black);

        QPainter painter(&frame);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(0, 0, graph);

        int position = 0;
        for (size_t i = 0; i < others.size(); ++i) {
            const bool last = i + 1 == others.size();
            if (useColumn) {
                const int h = last ? graphHeight - position
                                   : (int) std::lround((double) column * others[i].height() / others[i].width());
                painter.drawImage(QRect(graphWidth, position, column, h), others[i]);
                position += h;
            }
            else {
                const int w = last ? graphWidth - position
                                   : (int) std::lround((double) row * others[i].width() / others[i].height());
                painter.drawImage(QRect(position, graphHeight, w, row), others[i]);
                position += w;
            }
        }
    }

    // The first frame sets the size of the video. The sides have even lengths, and the width has a limit.
    if (!mSize.isValid()) {
        const double scale = std::min(1.0, 1920.0 / frame.width());
        mSize = QSize(std::max(2, ((int) std::lround(frame.width() * scale)) & ~1), std::max(2, ((int) std::lround(frame.height() * scale)) & ~1));
    }
    if (frame.size() != mSize) {
        frame = frame.scaled(mSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    return frame;
}

bool Recorder::start()
{
    if (mRecording) return false;

    const QString folder = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    const QString name = "informant-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".mp4";
    const QString chosen = QFileDialog::getSaveFileName(nullptr, tr("Record to"), QDir(folder).filePath(name),
        tr("Recording (*.mp4)"));
    if (chosen.isEmpty()) return false;

    const QFileInfo info(chosen);
    return begin(info.dir().filePath(info.completeBaseName()));
}

bool Recorder::begin(const QString &base)
{
    if (mRecording) return false;

    mBase = base;

    // The graph shows the window of time that the recording needs, so that the video has enough detail.
    mSavedSpan = mConfig->getViewTimeSpan();
    mSavedLive = mConfig->getViewLive();
    mConfig->setViewTimeSpan(std::clamp(mConfig->optionDouble("recordWindow"), 2.0, 60.0));
    mConfig->setViewLive(true);

    mStart = mDataStore->session().duration();
    mStarted = QDateTime::currentDateTime();
    mAudioSamples = 0;
    mLastTime = 0;
    mFrames = 0;
    mComposeMs = 0;
    mSize = QSize();
    mVideo.reset();

    if (mConfig->optionBool("recordVideo")) {
        // The first frame comes after the window has the new time span.
        QTimer::singleShot(300, this, [this]() {
            if (!mRecording || mVideo) return;

            const QImage first = composeFrame();
            if (first.isNull()) {
                emit message(QStringLiteral("Recording sound only: no picture of the window"));
                return;
            }

            QString error;
            const int fps = std::clamp(mConfig->optionInt("recordFps"), 5, 30);
            mVideo = VideoWriter::create(mBase + ".mp4", mSize.width(), mSize.height(), fps, Session::sampleRate, error);
            if (!mVideo) {
                emit message("Recording sound only: " + error);
                return;
            }

            // The sound before the first frame goes to the file, so that the sound and the picture keep the same clock.
            mLastTime = 0;
        });
    }

    mRecording = true;
    const int fps = std::clamp(mConfig->optionInt("recordFps"), 5, 30);
    mTimer.start(1000 / fps);
    emit recordingChanged();
    emit message(QStringLiteral("Recording"));
    return true;
}

bool Recorder::writeAudio(double until)
{
    // The sound of the file is exactly the sound of the session. The video file has the same samples.
    const long long total = (long long) std::floor((until - mStart) * Session::sampleRate);
    const long long count = total - mAudioSamples;
    if (count <= 0) return true;

    rpm::vector<float> raw;
    const double from = mStart + (double) mAudioSamples / Session::sampleRate;
    mDataStore->session().read(from, from + (double) count / Session::sampleRate, raw);

    std::vector<int16_t> samples(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        samples[i] = (int16_t) std::lround(std::clamp(raw[i], -1.0f, 1.0f) * 32767.0f);
    }
    mAudioSamples += (long long) raw.size();

    return mVideo ? mVideo->addAudio(samples.data(), samples.size()) : true;
}

void Recorder::tick()
{
    if (!mRecording || !mVideo) return;

    const double now = mDataStore->session().duration();
    const double time = now - mStart;

    // The capture is paused, or the sound did not advance.
    if (time <= mLastTime + 1e-3) return;

    const auto began = std::chrono::steady_clock::now();
    const QImage frame = composeFrame();
    if (!frame.isNull()) {
        mVideo->addFrame(frame, time);
        ++mFrames;
    }
    mComposeMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count();

    writeAudio(now);
    mLastTime = time;
    emit recordingChanged();
}

void Recorder::restoreView()
{
    mConfig->setViewTimeSpan(mSavedSpan);
    mConfig->setViewLive(mSavedLive);
}

void Recorder::stop()
{
    if (!mRecording) return;

    mTimer.stop();
    mRecording = false;

    const double end = mDataStore->session().duration();
    QStringList files;
    bool videoOk = true;

    if (mVideo) {
        writeAudio(end);
        emit message(QStringLiteral("Saving the video"));
        videoOk = mVideo->finish();
        mVideo.reset();
        files << QFileInfo(mBase + ".mp4").fileName();

        if (mFrames > 0) {
            std::cout << "Recorder] " << mFrames << " frames, " << (mComposeMs / mFrames) << " ms for each frame" << std::endl;
        }
    }

    restoreView();
    emit recordingChanged();

    const bool soundOk = mDataStore->session().exportWav(mBase + ".wav", mStart, end);
    if (soundOk) files << QFileInfo(mBase + ".wav").fileName();

    files << QFileInfo(mBase + ".md").fileName();
    const QString report = buildRecordingReport(*mConfig, *mDataStore, mStart, end, mStarted, files);
    QFile file(mBase + ".md");
    bool reportOk = false;
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&file);
        stream << report;
        reportOk = true;
    }

    if (!videoOk || !soundOk || !reportOk) {
        emit message(QStringLiteral("The recording has errors: some files are not complete"));
    }
    else {
        emit message("Saved " + QFileInfo(mBase).fileName());
    }
}
