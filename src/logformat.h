#ifndef LOG_FORMAT_H
#define LOG_FORMAT_H

#include <string>

// The rules of the log file, without the system calls, so a test can check them.
namespace LogFormat {

    // The folder of the log files on macOS. A user finds them in the Console app and in Finder.
    inline std::string folder(const std::string &home)
    {
        return home + "/Library/Logs/InFormant";
    }

    inline std::string file(const std::string &home, const std::string &name)
    {
        return folder(home) + "/" + name + ".log";
    }

    // The file of the last run before the rotation.
    inline std::string rotatedFile(const std::string &home, const std::string &name)
    {
        return folder(home) + "/" + name + ".1.log";
    }

    // A file that is larger than this size moves to the rotated file when the program starts.
    constexpr long long maximumFileBytes = 5LL * 1024 * 1024;

    inline bool needsRotation(long long size)
    {
        return size > maximumFileBytes;
    }

    // The longest line. A longer line breaks into two lines.
    constexpr size_t maximumLineBytes = 4096;

    // Builds lines from the chunks of text that arrive from the program. Each line starts with the stamp that the
    // caller gives, and the sink gets each finished line. A chunk can end inside a line, and a line can span chunks.
    class LineBuilder {
    public:
        template <typename Stamp, typename Sink>
        void feed(const char *data, size_t count, Stamp stamp, Sink sink)
        {
            for (size_t i = 0; i < count; ++i) {
                if (mLine.empty()) mLine = stamp();
                mLine.push_back(data[i]);
                if (data[i] == '\n' || mLine.size() >= maximumLineBytes) {
                    if (mLine.back() != '\n') mLine.push_back('\n');
                    sink(mLine);
                    mLine.clear();
                }
            }
        }

        // True when a line is open, with no end yet.
        bool open() const { return !mLine.empty(); }

        // Write an open line with an end, for the moment when no more text comes. The last line of a program that
        // stops with no new line at the end then stays in the log.
        template <typename Sink>
        void finish(Sink sink)
        {
            if (mLine.empty()) return;
            mLine.push_back('\n');
            sink(mLine);
            mLine.clear();
        }

    private:
        std::string mLine;
    };

}

#endif // LOG_FORMAT_H
