#include "modules/modules.h"
#include "analysis/analysis.h"
#include "context/contextmanager.h"
#include "file_logger.h"
#include <iostream>
#include <atomic>
#include <memory>
#include <chrono>
#include <csignal>
#include <thread>
# include <unistd.h>

#include <QApplication>
#include <QMessageBox>
#include <QSocketNotifier>
#include <QSurfaceFormat>


using namespace Module;
using namespace std::chrono_literals;

static std::atomic_bool signalCaught(false);
static std::atomic_int signalStatus;

// A signal handler must not call Qt. The handler can run in the middle of any code on the main
// thread, for example while a chart update holds a lock. The handler writes one byte to a pipe,
// and the event loop reads the byte and quits.
static int signalPipe[2] = { -1, -1 };

static void signalHandler(int signal) {
    signalCaught = true;
    signalStatus = signal;

    const char byte = (char) signal;
    if (signalPipe[1] >= 0) {
        (void) !write(signalPipe[1], &byte, 1);
    }
}

int Main::argc;
char **Main::argv;

std::unique_ptr<Main::ContextManager> Main::contextManager;


__attribute__((visibility ("default")))
int main(int argc, char **argv)
{

    openFileLogger("InFormant");

    // Request OpenGL 3.2 Core Profile for Apple Silicon compatibility.
    // macOS 2.1 compat profile doesn't support GL_R32F textures needed
    // for the spectrogram.
    QSurfaceFormat fmt;
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    if (pipe(signalPipe) != 0) {
        signalPipe[0] = signalPipe[1] = -1;
    }
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);

    Main::argc = argc;
    Main::argv = argv;

    Main::contextManager = std::make_unique<Main::ContextManager>(
            48'000,     // captureSampleRate
            50ms,       // playbackDuration
            48'000      // playbackSampleRate
    );

    // The application object exists now. Quit from the event loop when a signal arrives.
    std::unique_ptr<QSocketNotifier> signalNotifier;
    if (signalPipe[0] >= 0) {
        signalNotifier = std::make_unique<QSocketNotifier>(signalPipe[0], QSocketNotifier::Read);
        QObject::connect(signalNotifier.get(), &QSocketNotifier::activated, []() {
            char byte = 0;
            (void) !read(signalPipe[0], &byte, 1);
            std::cout << "Caught signal " << (byte == SIGINT ? "SIGINT" : "SIGTERM") << std::endl;
            QCoreApplication::quit();
        });
    }

    int ret = Main::contextManager->exec();

    signalNotifier.reset();

    // Destroy the Qt objects before the static destructors run. Qt thread storage no longer
    // exists at that point, and the window destructor crashes on macOS.
    Main::contextManager.reset();


    return ret;
}

