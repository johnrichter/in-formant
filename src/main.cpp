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
#ifndef _WIN32
# include <unistd.h>
#endif

#include <QApplication>
#include <QMessageBox>
#include <QSocketNotifier>
#include <QSurfaceFormat>

#ifdef _WIN32
# include <windows.h>
#endif

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

#ifndef _WIN32
    const char byte = (char) signal;
    if (signalPipe[1] >= 0) {
        (void) !write(signalPipe[1], &byte, 1);
    }
#else
    QMetaObject::invokeMethod(qGuiApp, "quit", Qt::QueuedConnection);
#endif
}

int Main::argc;
char **Main::argv;

std::unique_ptr<Main::ContextManager> Main::contextManager;

#ifdef _WIN32
#ifdef DEBUG_THREAD
void windowsDebugCallback(std::future<void> future);
#endif
#endif

#ifndef _MSC_VER
__attribute__((visibility ("default")))
#endif
int main(int argc, char **argv)
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(TopLevelExceptionHandler);
    srand(time(nullptr));
    #ifdef DEBUG_THREAD
    std::promise<void> exitSignal;
    std::future<void> exitSignalFuture = exitSignal.get_future();
    std::thread debugThread(windowsDebugCallback, std::move(exitSignalFuture));
    #endif
#endif

    openFileLogger("InFormant");

    // Request OpenGL 3.2 Core Profile for Apple Silicon compatibility.
    // macOS 2.1 compat profile doesn't support GL_R32F textures needed
    // for the spectrogram.
    QSurfaceFormat fmt;
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

#ifndef _WIN32
    if (pipe(signalPipe) != 0) {
        signalPipe[0] = signalPipe[1] = -1;
    }
#endif
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
#ifndef _WIN32
    if (signalPipe[0] >= 0) {
        signalNotifier = std::make_unique<QSocketNotifier>(signalPipe[0], QSocketNotifier::Read);
        QObject::connect(signalNotifier.get(), &QSocketNotifier::activated, []() {
            char byte = 0;
            (void) !read(signalPipe[0], &byte, 1);
            std::cout << "Caught signal " << (byte == SIGINT ? "SIGINT" : "SIGTERM") << std::endl;
            QCoreApplication::quit();
        });
    }
#endif

    int ret = Main::contextManager->exec();

    signalNotifier.reset();

    // Destroy the Qt objects before the static destructors run. Qt thread storage no longer
    // exists at that point, and the window destructor crashes on macOS.
    Main::contextManager.reset();

    #ifdef DEBUG_THREAD
    exitSignal.set_value();
    if (debugThread.joinable())
        debugThread.join();
    #endif

    return ret;
}

