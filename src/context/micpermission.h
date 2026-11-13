#ifndef MAIN_MIC_PERMISSION_H
#define MAIN_MIC_PERMISSION_H

#include <functional>

#include "micaccess.h"

namespace Main {

    // The permission of the system to use the microphone. The calls use AVFoundation directly. The permission
    // plugin of Qt refuses to ask in a build that starts from Finder, so the program does not use the plugin.

    // The state of the permission now. The call does not show any question.
    MicAccess systemMicAccess();

    // Show the question of the system when the state is Undetermined. The function receives true when the access is
    // allowed. The function runs on a thread of the system. It must only post work to another thread.
    void requestSystemMicAccess(std::function<void(bool allowed)> answered);

    // Open the page of the microphone in the System Settings.
    void openMicrophoneSettings();

}

#endif // MAIN_MIC_PERMISSION_H
