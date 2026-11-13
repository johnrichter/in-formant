#ifndef CONFIGMIGRATION_H
#define CONFIGMIGRATION_H

#include <cstdint>
#include <toml++/toml.h>

namespace Main {

// The version of the configuration file that this program writes.
constexpr int64_t kConfigVersion = 8;

// The default values of the statistics before version 4, and the median bit that version 4 adds.
constexpr int64_t kOldStatsMask = 0x1 | 0x2 | 0x8 | 0x40 | 0x80;
constexpr int64_t kStatMedianBit = 0x10;

namespace detail {

// Replace a number only when it still has the old default value. A value that the user changed stays.
inline void replaceOldDefault(toml::table &sub, const char *key, double oldValue, double newValue)
{
    if (const auto v = sub[key].value<double>()) {
        if (*v == oldValue) {
            sub.insert_or_assign(key, newValue);
        }
    }
}

} // namespace detail

// The file stores each default at the first exit, so a new default has no effect on an existing file.
// Each version of the file applies its new defaults one time, then the version number goes up.
// The sub tables "analysis", "view", "options" and "meta" must exist before the call.
inline void migrateConfig(toml::table &tbl)
{
    const int64_t fileVersion = tbl["meta"]["version"].value_or<int64_t>(0);

    if (fileVersion < 2) {
        tbl["analysis"].as_table()->insert_or_assign("granularity", 10.0);
        // Closer frames give smoother lines in the tracks.
        tbl["analysis"].as_table()->insert_or_assign("pitchSpacing", 20.0);
        tbl["analysis"].as_table()->insert_or_assign("formantSpacing", 40.0);
        tbl["view"].as_table()->insert_or_assign("timeSpan", 15.0);
    }
    if (fileVersion < 3) {
        // The glottal flow estimate follows the sound at a high rate.
        tbl["analysis"].as_table()->insert_or_assign("oscilloscopeSpacing", 20.0);
    }
    if (fileVersion < 4) {
        toml::table &options = *tbl["options"].as_table();
        detail::replaceOldDefault(options, "lineWidth", 2.5, 2.0);
        detail::replaceOldDefault(options, "dotRadius", 3.0, 2.0);
        if (const auto mask = options["statsMask"].value<int64_t>()) {
            if (*mask == kOldStatsMask) {
                options.insert_or_assign("statsMask", kOldStatsMask | kStatMedianBit);
            }
        }
        // The default top of the spectrogram view is 5500 Hz. The limit of the formant tracker stays.
        if (const auto top = tbl["view"]["maxFrequency"].value<int64_t>()) {
            if (*top == 6000) {
                tbl["view"].as_table()->insert_or_assign("maxFrequency", 5500);
            }
        }
    }

    if (fileVersion < 5) {
        // One style for all tracks becomes one style for each track. The old values were
        // 0 dots, 1 lines, 2 lines and dots. The new values are bits: 1 dots, 2 line.
        toml::table &options = *tbl["options"].as_table();
        if (const auto old = options["trackStyle"].value<int64_t>()) {
            const int64_t mask = *old == 0 ? 1 : (*old == 1 ? 2 : 3);
            for (const char *key : { "trackStylePitch", "trackStyleF1", "trackStyleF2", "trackStyleF3", "trackStyleF4" }) {
                if (!options[key]) options.insert(key, mask);
            }
            options.erase("trackStyle");
        }
    }

    if (fileVersion < 6) {
        // The statistics boxes show by default. The program wrote the old default "off" to the file.
        toml::table &options = *tbl["options"].as_table();
        if (const auto shown = options["showStats"].value<bool>()) {
            if (!*shown) options.insert_or_assign("showStats", true);
        }
    }

    if (fileVersion < 7) {
        // The boxes of the tracks stand at the left of the graph by default. The program wrote the old default "bottom".
        toml::table &options = *tbl["options"].as_table();
        if (const auto side = options["readoutSide"].value<std::string>()) {
            if (*side == "bottom") options.insert_or_assign("readoutSide", "left");
        }
    }

    if (fileVersion < 8) {
        // One switch for all formant tracks becomes one switch for each track. The old switch "off" hides all four.
        if (const auto shown = tbl["view"]["showFormants"].value<bool>()) {
            if (!*shown) {
                toml::table &options = *tbl["options"].as_table();
                for (const char *key : { "showTrackF1", "showTrackF2", "showTrackF3", "showTrackF4" }) {
                    options.insert_or_assign(key, false);
                }
            }
        }
    }

    tbl["meta"].as_table()->insert_or_assign("version", kConfigVersion);
}

} // namespace Main

#endif // CONFIGMIGRATION_H
