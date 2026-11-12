#ifndef MAIN_CONTEXT_VIDEO_WRITER_H
#define MAIN_CONTEXT_VIDEO_WRITER_H

#include <cstddef>
#include <cstdint>
#include <memory>

#include <QImage>
#include <QString>

namespace Main {

    // Writes an MP4 file with H.264 video and AAC sound. The sound has a fixed sample rate, and the
    // video frames have times in seconds. All calls come from one thread.
    class VideoWriter {
    public:
        virtual ~VideoWriter() = default;

        // The result is null, and the error has the reason, when the system cannot write video.
        static std::unique_ptr<VideoWriter> create(const QString &path, int width, int height, int framesPerSecond,
                                                   int audioRate, QString &error);

        // Add one frame. The image has the size that the writer got at its creation. The call
        // drops the frame when the encoder is busy.
        virtual bool addFrame(const QImage &image, double time) = 0;

        // Add mono 16-bit samples. The samples continue the samples of the earlier calls.
        virtual bool addAudio(const int16_t *samples, size_t count) = 0;

        // Write the end of the file. The call waits until the file is complete.
        virtual bool finish() = 0;
    };

}

#endif // MAIN_CONTEXT_VIDEO_WRITER_H
