#ifndef MAIN_DEVICE_WATCHER_H
#define MAIN_DEVICE_WATCHER_H

#include <cstdint>
#include <functional>
#include <memory>

namespace Main {

    // The identity of the default input device of the system. Zero means that the system has no
    // such device, or that the platform cannot tell.
    uint32_t systemDefaultInputDeviceId();

    // Tells its owner when the default input device or the list of audio devices changes.
    // The function runs on a thread of the system. It must only post work to another thread.
    class DeviceWatcher {
    public:
        explicit DeviceWatcher(std::function<void()> onChange);
        ~DeviceWatcher();

        DeviceWatcher(const DeviceWatcher &) = delete;
        DeviceWatcher &operator=(const DeviceWatcher &) = delete;

    private:
        struct Impl;
        std::unique_ptr<Impl> mImpl;
    };

}

#endif // MAIN_DEVICE_WATCHER_H
