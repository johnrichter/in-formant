#ifndef MAIN_VOWEL_PRESETS_H
#define MAIN_VOWEL_PRESETS_H

#include <array>

namespace Main {

    // The first three formants of ten vowels for an average adult man and an average adult woman. The values
    // are the averages of Peterson and Barney (1952), American English. The study has no F4, so each voice
    // has one typical F4.
    struct VowelPreset {
        const char *name;
        std::array<int, 3> male;
        std::array<int, 3> female;
    };

    inline constexpr std::array<VowelPreset, 10> vowelPresets = {{
        { "ee, as in see",     { 270, 2290, 3010 }, { 310, 2790, 3310 } },
        { "ih, as in sit",     { 390, 1990, 2550 }, { 430, 2480, 3070 } },
        { "eh, as in bed",     { 530, 1840, 2480 }, { 610, 2330, 2990 } },
        { "ae, as in cat",     { 660, 1720, 2410 }, { 860, 2050, 2850 } },
        { "ah, as in father",  { 730, 1090, 2440 }, { 850, 1220, 2810 } },
        { "aw, as in thought", { 570, 840, 2410 },  { 590, 920, 2710 } },
        { "uh, as in book",    { 440, 1020, 2240 }, { 470, 1160, 2680 } },
        { "oo, as in food",    { 300, 870, 2240 },  { 370, 950, 2670 } },
        { "u, as in cup",      { 640, 1190, 2390 }, { 760, 1400, 2780 } },
        { "er, as in bird",    { 490, 1350, 1690 }, { 500, 1640, 1960 } },
    }};

    struct VoicePreset {
        const char *name;
        const char *key;
        int pitch;
        int f4;
    };

    inline constexpr std::array<VoicePreset, 2> voicePresets = {{
        { "Lower voice (average man)", "male", 120, 3500 },
        { "Higher voice (average woman)", "female", 210, 4000 },
    }};

    // The bandwidths of F1 to F4 for a preset, in Hz.
    inline constexpr std::array<int, 4> presetBandwidths = { 60, 90, 120, 150 };

}

#endif // MAIN_VOWEL_PRESETS_H
