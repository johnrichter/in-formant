#ifndef MAIN_CONTEXT_AUDIO_FILE_H
#define MAIN_CONTEXT_AUDIO_FILE_H

#include <functional>

#include <QString>

namespace Main {

    // Read the sound of an audio file as mono samples from -1 to 1. The function calls the sink with
    // each block of samples and the sample rate of the block. The sink returns false to stop the read.
    // The result is false when the file cannot be read, and the error has the reason. The macOS version
    // reads each format of the system. The other versions read WAV files.
    bool decodeAudioFile(const QString &path,
                         const std::function<bool(const float *samples, size_t count, double sampleRate)> &sink,
                         QString &error);

}

#endif // MAIN_CONTEXT_AUDIO_FILE_H
