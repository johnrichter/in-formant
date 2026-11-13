#include <gtest/gtest.h>

#include "../src/context/micaccess.h"

#include <string>

using namespace Main;

// The first press of play asks the system for the microphone. A denied access stays paused and says what to do.
TEST(MicAccess, TP_CAP_17_TheStepFollowsTheStateOfThePermission)
{
    EXPECT_EQ(micStep(MicAccess::Granted), MicStep::Start);
    EXPECT_EQ(micStep(MicAccess::Undetermined), MicStep::Ask);
    EXPECT_EQ(micStep(MicAccess::Denied), MicStep::ShowDenied);
}

TEST(MicAccess, TP_CAP_17_TheDeniedMessageNamesTheSettingsAndTheApp)
{
    const std::string text = micDeniedMessage();
    EXPECT_NE(text.find("System Settings"), std::string::npos);
    EXPECT_NE(text.find("Microphone"), std::string::npos);
    EXPECT_NE(text.find("InFormant"), std::string::npos);
    EXPECT_LE(text.size(), 140u);
}
