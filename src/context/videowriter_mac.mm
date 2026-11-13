#include "videowriter.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include <algorithm>
#include <cstring>
#include <unistd.h>

using namespace Main;

namespace {

class MacVideoWriter : public VideoWriter {
public:
    bool open(const QString &path, int width, int height, int fps, int audioRate, QString &error)
    {
        mWidth = width;
        mHeight = height;
        mAudioRate = audioRate;

        NSString *file = [NSString stringWithUTF8String:path.toUtf8().constData()];
        NSURL *url = [NSURL fileURLWithPath:file];
        [[NSFileManager defaultManager] removeItemAtURL:url error:nil];

        NSError *failure = nil;
        mWriter = [[AVAssetWriter alloc] initWithURL:url fileType:AVFileTypeMPEG4 error:&failure];
        if (mWriter == nil) {
            error = QString::fromNSString(failure.localizedDescription ?: @"The video file cannot be created");
            return false;
        }

        // About 0.07 bits for each pixel in each frame, which is enough for flat colors and thin lines.
        const double bitrate = std::max(2.0e6, 0.07 * width * height * fps);
        NSDictionary *videoSettings = @{
            AVVideoCodecKey: AVVideoCodecTypeH264,
            AVVideoWidthKey: @(width),
            AVVideoHeightKey: @(height),
            AVVideoCompressionPropertiesKey: @{
                AVVideoAverageBitRateKey: @(bitrate),
                AVVideoExpectedSourceFrameRateKey: @(fps),
                AVVideoProfileLevelKey: AVVideoProfileLevelH264HighAutoLevel,
            },
        };
        mVideo = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];
        mVideo.expectsMediaDataInRealTime = YES;

        NSDictionary *bufferSettings = @{
            (id) kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
            (id) kCVPixelBufferWidthKey: @(width),
            (id) kCVPixelBufferHeightKey: @(height),
        };
        mAdaptor = [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:mVideo
                                                                                  sourcePixelBufferAttributes:bufferSettings];

        NSDictionary *audioSettings = @{
            AVFormatIDKey: @(kAudioFormatMPEG4AAC),
            AVSampleRateKey: @(audioRate),
            AVNumberOfChannelsKey: @1,
            AVEncoderBitRateKey: @64000,
        };
        mAudio = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio outputSettings:audioSettings];
        mAudio.expectsMediaDataInRealTime = YES;

        if (![mWriter canAddInput:mVideo] || ![mWriter canAddInput:mAudio]) {
            error = QStringLiteral("The system cannot write this video format");
            return false;
        }
        [mWriter addInput:mVideo];
        [mWriter addInput:mAudio];

        if (![mWriter startWriting]) {
            error = QString::fromNSString(mWriter.error.localizedDescription ?: @"The video file cannot be written");
            return false;
        }
        [mWriter startSessionAtSourceTime:kCMTimeZero];
        return true;
    }

    bool addFrame(const QImage &source, double time) override
    {
        if (mWriter.status != AVAssetWriterStatusWriting) return false;
        if (!mVideo.readyForMoreMediaData) return true;

        const QImage image = source.format() == QImage::Format_RGB32 ? source : source.convertToFormat(QImage::Format_RGB32);
        if (image.width() != mWidth || image.height() != mHeight) return false;

        CVPixelBufferRef buffer = nullptr;
        if (CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, mAdaptor.pixelBufferPool, &buffer) != kCVReturnSuccess) {
            return true;
        }

        CVPixelBufferLockBaseAddress(buffer, 0);
        uint8_t *target = static_cast<uint8_t *>(CVPixelBufferGetBaseAddress(buffer));
        const size_t stride = CVPixelBufferGetBytesPerRow(buffer);
        for (int y = 0; y < mHeight; ++y) {
            std::memcpy(target + y * stride, image.constScanLine(y), (size_t) mWidth * 4);
        }
        CVPixelBufferUnlockBaseAddress(buffer, 0);

        const BOOL ok = [mAdaptor appendPixelBuffer:buffer withPresentationTime:CMTimeMakeWithSeconds(time, 600)];
        CVPixelBufferRelease(buffer);
        return ok;
    }

    bool addAudio(const int16_t *samples, size_t count) override
    {
        if (count == 0) return true;
        if (mWriter.status != AVAssetWriterStatusWriting) return false;

        // The sound must not have gaps, so the call waits a short time for the encoder.
        for (int i = 0; i < 100 && !mAudio.readyForMoreMediaData; ++i) usleep(2000);
        if (!mAudio.readyForMoreMediaData) return false;

        AudioStreamBasicDescription format = {};
        format.mSampleRate = mAudioRate;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        format.mBytesPerPacket = 2;
        format.mFramesPerPacket = 1;
        format.mBytesPerFrame = 2;
        format.mChannelsPerFrame = 1;
        format.mBitsPerChannel = 16;

        CMAudioFormatDescriptionRef description = nullptr;
        if (CMAudioFormatDescriptionCreate(kCFAllocatorDefault, &format, 0, nullptr, 0, nullptr, nullptr, &description) != noErr) {
            return false;
        }

        const size_t bytes = count * 2;
        CMBlockBufferRef block = nullptr;
        OSStatus status = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, nullptr, bytes, kCFAllocatorDefault, nullptr, 0,
                                                             bytes, kCMBlockBufferAssureMemoryNowFlag, &block);
        if (status == noErr) status = CMBlockBufferReplaceDataBytes(samples, block, 0, bytes);

        CMSampleBufferRef buffer = nullptr;
        if (status == noErr) {
            status = CMAudioSampleBufferCreateReadyWithPacketDescriptions(kCFAllocatorDefault, block, description, (CMItemCount) count,
                CMTimeMake(mAudioSamples, mAudioRate), nullptr, &buffer);
        }

        bool ok = false;
        if (status == noErr) {
            ok = [mAudio appendSampleBuffer:buffer];
            mAudioSamples += (long long) count;
        }

        if (buffer) CFRelease(buffer);
        if (block) CFRelease(block);
        CFRelease(description);
        return ok;
    }

    bool finish() override
    {
        if (mWriter.status != AVAssetWriterStatusWriting) return mWriter.status == AVAssetWriterStatusCompleted;

        [mVideo markAsFinished];
        [mAudio markAsFinished];

        dispatch_semaphore_t done = dispatch_semaphore_create(0);
        [mWriter finishWritingWithCompletionHandler:^{ dispatch_semaphore_signal(done); }];
        dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 120 * NSEC_PER_SEC));
        return mWriter.status == AVAssetWriterStatusCompleted;
    }

private:
    AVAssetWriter *mWriter = nil;
    AVAssetWriterInput *mVideo = nil;
    AVAssetWriterInput *mAudio = nil;
    AVAssetWriterInputPixelBufferAdaptor *mAdaptor = nil;
    int mWidth = 0, mHeight = 0, mAudioRate = 0;
    long long mAudioSamples = 0;
};

}

std::unique_ptr<VideoWriter> VideoWriter::create(const QString &path, int width, int height, int fps, int audioRate, QString &error)
{
    auto writer = std::make_unique<MacVideoWriter>();
    if (!writer->open(path, width, height, fps, audioRate, error)) return nullptr;
    return writer;
}
