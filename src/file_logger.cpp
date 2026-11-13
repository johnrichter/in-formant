#include "file_logger.h"
#include "logformat.h"

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <string>

static std::string homeFolder()
{
    if (const char *home = getenv("HOME")) {
        if (*home != '\0') return home;
    }
    if (const struct passwd *entry = getpwuid(getuid())) return entry->pw_dir;
    return std::string();
}

// Opens the log file in ~/Library/Logs/InFormant. A file that is too large moves to a second file first.
// The function returns -1 when no file can be opened.
static int openLogFile(const char *name)
{
    const std::string home = homeFolder();
    if (home.empty()) return -1;

    mkdir((home + "/Library").c_str(), 0700);
    mkdir((home + "/Library/Logs").c_str(), 0755);
    mkdir(LogFormat::folder(home).c_str(), 0755);

    const std::string path = LogFormat::file(home, name);
    struct stat info;
    if (stat(path.c_str(), &info) == 0 && LogFormat::needsRotation((long long) info.st_size)) {
        rename(path.c_str(), LogFormat::rotatedFile(home, name).c_str());
    }

    return open(path.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
}

static std::string timeStamp()
{
    const time_t timer = time(NULL);
    struct tm info;
    localtime_r(&timer, &info);
    char text[32];
    strftime(text, sizeof(text), "[%Y-%m-%d %H:%M:%S] ", &info);
    return text;
}

void openFileLogger(const char *name) {
    int fd = openLogFile(name);
    if (fd == -1) {
        perror("open");
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        perror("pipe");
        close(fd);
        return;
    }

    pid_t cpid = fork();
    if (cpid == -1) {
        perror("fork");
        close(fd);
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (cpid == 0) {
        close(pipefd[1]);

        LogFormat::LineBuilder builder;
        auto sink = [fd](const std::string &line) {
            (void) !write(STDOUT_FILENO, line.data(), line.size());
            (void) !write(fd, line.data(), line.size());
        };

        char buffer[4096];
        ssize_t count;
        // The end of the file means that the application closed its end of the pipe.
        // Stop, or this process stays alive after the application exits.
        while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            builder.feed(buffer, (size_t) count, timeStamp, sink);
        }

        close(fd);
        close(pipefd[0]);
        // This process is a copy of the program. Leave without the static destructors of the copy.
        // Those of Qt throw an exception at exit, and the system then reports a crash.
        _exit(EXIT_SUCCESS);
    }
    else {
        close(fd);
        close(pipefd[0]);
        
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
    }
}
