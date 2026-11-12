#include "devicewatcher.h"

using namespace Main;

// Other platforms do not tell the application when a device changes.

uint32_t Main::systemDefaultInputDeviceId()
{
    return 0;
}

struct DeviceWatcher::Impl {};

DeviceWatcher::DeviceWatcher(std::function<void()>)
    : mImpl(std::make_unique<Impl>())
{
}

DeviceWatcher::~DeviceWatcher() = default;
