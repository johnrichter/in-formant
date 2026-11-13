#include "micpermission.h"

#import <AVFoundation/AVFoundation.h>
#import <AppKit/AppKit.h>

using namespace Main;

MicAccess Main::systemMicAccess()
{
    switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio]) {
    case AVAuthorizationStatusAuthorized: return MicAccess::Granted;
    case AVAuthorizationStatusNotDetermined: return MicAccess::Undetermined;
    case AVAuthorizationStatusDenied:
    case AVAuthorizationStatusRestricted: break;
    }
    return MicAccess::Denied;
}

void Main::requestSystemMicAccess(std::function<void(bool)> answered)
{
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
        answered(granted == YES);
    }];
}

void Main::openMicrophoneSettings()
{
    NSURL *url = [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone"];
    [[NSWorkspace sharedWorkspace] openURL:url];
}
