#ifndef MAIN_MIC_ACCESS_H
#define MAIN_MIC_ACCESS_H

namespace Main {

    // The state of the permission to use the microphone. macOS keeps it for each application. The state is
    // Undetermined until the user answers the question of the system one time.
    enum class MicAccess {
        Undetermined,
        Granted,
        Denied,
    };

    // What the program does when the user presses play.
    enum class MicStep {
        Start,       // The capture starts at once.
        Ask,         // The program asks the system. The system shows its question, and the answer comes later.
        ShowDenied,  // The program stays paused and tells the user what to do.
    };

    inline MicStep micStep(MicAccess access)
    {
        switch (access) {
        case MicAccess::Granted: return MicStep::Start;
        case MicAccess::Undetermined: return MicStep::Ask;
        case MicAccess::Denied: break;
        }
        return MicStep::ShowDenied;
    }

    // The text for a user who denied the access. A denied access stays denied until the user changes it in the
    // System Settings, and the system does not ask again.
    inline const char *micDeniedMessage()
    {
        return "Microphone access is off. Open System Settings, Privacy and Security, Microphone, and switch on InFormant.";
    }

}

#endif // MAIN_MIC_ACCESS_H
