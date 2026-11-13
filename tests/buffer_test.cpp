#include <gtest/gtest.h>

#include "../src/modules/audio/buffer/buffer.h"

#include <vector>

using Module::Audio::Buffer;

namespace {

// The queue rounds its space up to whole blocks, so the real space is a little more than the capacity.
// Push until the first drop, in blocks. The function returns the number of samples that were pushed.
long long fill(Buffer &buffer, int blockSize)
{
    std::vector<float> block((size_t) blockSize, 0.25f);
    long long pushed = 0;
    while (buffer.droppedSamples() == 0 && pushed < 4LL * Buffer::capacity) {
        buffer.push(block.data(), blockSize);
        pushed += blockSize;
    }
    return pushed;
}

}

// The audio thread of the system calls push. It must never allocate memory and never block. The buffer
// reserves its space when it starts, and a full buffer drops samples and counts them.
TEST(CaptureBuffer, TP_CAP_16_ThePushTakesAtLeastTheCapacityBeforeItDrops)
{
    Buffer buffer(48000);
    std::vector<float> block(4096, 0.25f);
    int pushed = 0;
    while (pushed + (int) block.size() <= Buffer::capacity) {
        buffer.push(block.data(), (int) block.size());
        pushed += (int) block.size();
    }
    EXPECT_EQ(buffer.droppedSamples(), 0);
    EXPECT_EQ(buffer.getLength(), pushed);
}

TEST(CaptureBuffer, TP_CAP_16_AFullBufferDropsSamplesAndCountsThemExactly)
{
    Buffer buffer(48000);
    const long long pushed = fill(buffer, 1000);
    ASSERT_GT(buffer.droppedSamples(), 0);

    // Each sample is in the buffer or in the count of the drops. None is missing.
    EXPECT_EQ(buffer.getLength() + buffer.droppedSamples(), pushed);
    EXPECT_GE(buffer.getLength(), Buffer::capacity);
}

TEST(CaptureBuffer, TP_CAP_16_TheSamplesKeepTheirOrderAndNothingIsLostInNormalUse)
{
    Buffer buffer(16000);
    std::vector<float> in(5000);
    for (size_t i = 0; i < in.size(); ++i) in[i] = (float) i;

    buffer.push(in.data(), (int) in.size());
    EXPECT_EQ(buffer.droppedSamples(), 0);

    std::vector<double> out(in.size());
    buffer.pull(out.data(), (int) out.size());
    for (size_t i = 0; i < in.size(); ++i) ASSERT_EQ(out[i], (double) i) << i;
    EXPECT_EQ(buffer.getLength(), 0);
}

// After the reader catches up, the buffer takes new samples again.
TEST(CaptureBuffer, TP_CAP_16_TheBufferTakesSamplesAgainAfterTheReaderCatchesUp)
{
    Buffer buffer(48000);
    fill(buffer, 1000);
    const long long dropped = buffer.droppedSamples();
    ASSERT_GT(dropped, 0);

    std::vector<double> sink((size_t) buffer.getLength());
    buffer.pull(sink.data(), (int) sink.size());
    EXPECT_EQ(buffer.getLength(), 0);

    std::vector<float> block(100, 1.0f);
    buffer.push(block.data(), (int) block.size());
    EXPECT_EQ(buffer.droppedSamples(), dropped);
    EXPECT_EQ(buffer.getLength(), 100);
}
