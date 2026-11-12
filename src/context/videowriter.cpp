#include "videowriter.h"

using namespace Main;

// The systems other than macOS have no video encoder in the program.
std::unique_ptr<VideoWriter> VideoWriter::create(const QString &, int, int, int, int, QString &error)
{
    error = QStringLiteral("Video recording needs macOS");
    return nullptr;
}
