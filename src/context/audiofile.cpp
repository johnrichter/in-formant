#include "audiofile.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <QFile>

#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace Main {

#ifdef __APPLE__

bool decodeAudioFile(const QString &path,
                     const std::function<bool(const float *, size_t, double)> &sink,
                     QString &error)
{
    const QByteArray utf8 = path.toUtf8();
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        nullptr, reinterpret_cast<const UInt8 *>(utf8.constData()), (CFIndex) utf8.size(), false);
    if (url == nullptr) {
        error = QStringLiteral("The file name is not valid");
        return false;
    }

    ExtAudioFileRef file = nullptr;
    OSStatus status = ExtAudioFileOpenURL(url, &file);
    CFRelease(url);
    if (status != noErr || file == nullptr) {
        error = QStringLiteral("The system cannot open this file");
        return false;
    }

    // The system converts the sound to mono, and to the sample rate of the session.
    constexpr double rate = 32000.0;
    AudioStreamBasicDescription format = {};
    format.mSampleRate = rate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel = 32;
    format.mChannelsPerFrame = 1;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = 4;
    format.mBytesPerPacket = 4;

    status = ExtAudioFileSetProperty(file, kExtAudioFileProperty_ClientDataFormat, sizeof(format), &format);
    if (status != noErr) {
        ExtAudioFileDispose(file);
        error = QStringLiteral("The system cannot convert this file");
        return false;
    }

    std::vector<float> block(1 << 16);
    bool ok = true;
    for (;;) {
        AudioBufferList list;
        list.mNumberBuffers = 1;
        list.mBuffers[0].mNumberChannels = 1;
        list.mBuffers[0].mDataByteSize = (UInt32) (block.size() * sizeof(float));
        list.mBuffers[0].mData = block.data();

        UInt32 frames = (UInt32) block.size();
        status = ExtAudioFileRead(file, &frames, &list);
        if (status != noErr) {
            error = QStringLiteral("The system cannot read this file");
            ok = false;
            break;
        }
        if (frames == 0) break;
        if (!sink(block.data(), frames, rate)) break;
    }

    ExtAudioFileDispose(file);
    return ok;
}

#else

namespace {

uint32_t le32(const unsigned char *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t) p[3] << 24); }
uint16_t le16(const unsigned char *p) { return (uint16_t) (p[0] | (p[1] << 8)); }

}

bool decodeAudioFile(const QString &path,
                     const std::function<bool(const float *, size_t, double)> &sink,
                     QString &error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("The file cannot be opened");
        return false;
    }

    unsigned char header[12];
    if (file.read(reinterpret_cast<char *>(header), 12) != 12 || std::memcmp(header, "RIFF", 4) != 0
        || std::memcmp(header + 8, "WAVE", 4) != 0) {
        error = QStringLiteral("Only WAV files can be read on this system");
        return false;
    }

    int format = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    for (;;) {
        unsigned char chunk[8];
        if (file.read(reinterpret_cast<char *>(chunk), 8) != 8) {
            error = QStringLiteral("The WAV file has no sound data");
            return false;
        }
        const uint32_t size = le32(chunk + 4);

        if (std::memcmp(chunk, "fmt ", 4) == 0) {
            std::vector<unsigned char> body(std::max<uint32_t>(size, 16));
            if (file.read(reinterpret_cast<char *>(body.data()), size) != (qint64) size) {
                error = QStringLiteral("The WAV file is damaged");
                return false;
            }
            format = le16(body.data());
            channels = le16(body.data() + 2);
            rate = le32(body.data() + 4);
            bits = le16(body.data() + 14);
            if (format == 0xFFFE && size >= 26) format = le16(body.data() + 24);
        }
        else if (std::memcmp(chunk, "data", 4) == 0) {
            break;
        }
        else if (!file.seek(file.pos() + size + (size & 1))) {
            error = QStringLiteral("The WAV file is damaged");
            return false;
        }
    }

    const bool isFloat = format == 3 && bits == 32;
    const bool isInt = format == 1 && (bits == 16 || bits == 24 || bits == 32);
    if (channels < 1 || rate == 0 || !(isFloat || isInt)) {
        error = QStringLiteral("This WAV format is not supported");
        return false;
    }

    const int bytes = bits / 8;
    const size_t frameBytes = (size_t) bytes * channels;
    std::vector<unsigned char> raw(frameBytes * 8192);
    std::vector<float> mono(8192);

    for (;;) {
        const qint64 got = file.read(reinterpret_cast<char *>(raw.data()), (qint64) raw.size());
        if (got <= 0) break;

        const size_t frames = (size_t) got / frameBytes;
        for (size_t i = 0; i < frames; ++i) {
            double sum = 0;
            for (int c = 0; c < channels; ++c) {
                const unsigned char *p = raw.data() + i * frameBytes + (size_t) c * bytes;
                if (isFloat) {
                    float v;
                    std::memcpy(&v, p, 4);
                    sum += v;
                }
                else if (bits == 16) {
                    sum += (int16_t) le16(p) / 32768.0;
                }
                else if (bits == 24) {
                    int32_t v = (p[0] << 8) | (p[1] << 16) | ((int32_t) p[2] << 24);
                    sum += v / 2147483648.0;
                }
                else {
                    sum += (int32_t) le32(p) / 2147483648.0;
                }
            }
            mono[i] = (float) (sum / channels);
        }

        if (frames > 0 && !sink(mono.data(), frames, (double) rate)) break;
    }

    return true;
}

#endif

}
