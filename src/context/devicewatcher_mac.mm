#include "devicewatcher.h"

#import <CoreAudio/CoreAudio.h>
#import <dispatch/dispatch.h>

using namespace Main;

namespace {

    const AudioObjectPropertyAddress defaultInputAddress = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    const AudioObjectPropertyAddress devicesAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

}

uint32_t Main::systemDefaultInputDeviceId()
{
    AudioObjectID id = kAudioObjectUnknown;
    UInt32 size = sizeof(id);
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &defaultInputAddress, 0, nullptr, &size, &id) != noErr) {
        return 0;
    }
    return id;
}

struct DeviceWatcher::Impl {
    std::function<void()> onChange;
    dispatch_queue_t queue = nullptr;
    AudioObjectPropertyListenerBlock block = nil;
};

DeviceWatcher::DeviceWatcher(std::function<void()> onChange)
    : mImpl(std::make_unique<Impl>())
{
    Impl *impl = mImpl.get();
    impl->onChange = std::move(onChange);
    impl->queue = dispatch_queue_create("informant.devicewatcher", DISPATCH_QUEUE_SERIAL);
    impl->block = ^(UInt32, const AudioObjectPropertyAddress *) {
        if (impl->onChange) impl->onChange();
    };

    AudioObjectAddPropertyListenerBlock(kAudioObjectSystemObject, &defaultInputAddress, impl->queue, impl->block);
    AudioObjectAddPropertyListenerBlock(kAudioObjectSystemObject, &devicesAddress, impl->queue, impl->block);
}

DeviceWatcher::~DeviceWatcher()
{
    Impl *impl = mImpl.get();

    AudioObjectRemovePropertyListenerBlock(kAudioObjectSystemObject, &defaultInputAddress, impl->queue, impl->block);
    AudioObjectRemovePropertyListenerBlock(kAudioObjectSystemObject, &devicesAddress, impl->queue, impl->block);

    // Wait for a call that already runs, so that none runs after this object ends.
    dispatch_sync(impl->queue, ^{});
}
