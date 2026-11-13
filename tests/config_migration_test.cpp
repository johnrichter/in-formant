#include <gtest/gtest.h>
#include "../src/context/configmigration.h"

using namespace Main;

static toml::table makeTable(const char *text)
{
    toml::table tbl = toml::parse(text);
    for (const char *name : { "analysis", "view", "options", "meta" }) {
        if (!tbl[name]) tbl.insert(name, toml::table());
    }
    return tbl;
}

// An old file that holds the old defaults gets the new defaults.
TEST(ConfigMigration, AnOldFileThatHoldsTheOldDefaultsGetsTheNewDefaults)
{
    toml::table tbl = makeTable(
        "[meta]\nversion = 3\n"
        "[view]\nmaxFrequency = 6000\n"
        "[options]\nlineWidth = 2.5\ndotRadius = 3.0\nstatsMask = 203\n");
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["lineWidth"].value<double>() == 2.0);
    EXPECT_TRUE(tbl["options"]["dotRadius"].value<double>() == 2.0);
    EXPECT_TRUE(tbl["options"]["statsMask"].value<int64_t>() == (kOldStatsMask | kStatMedianBit));
    EXPECT_TRUE(tbl["view"]["maxFrequency"].value<int64_t>() == 5500);
    EXPECT_TRUE(tbl["meta"]["version"].value<int64_t>() == kConfigVersion);
}

// A value that the user changed stays.
TEST(ConfigMigration, AValueThatTheUserChangedStays)
{
    toml::table tbl = makeTable(
        "[meta]\nversion = 3\n"
        "[view]\nmaxFrequency = 8000\n"
        "[options]\nlineWidth = 4.0\ndotRadius = 1.5\nstatsMask = 7\n");
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["lineWidth"].value<double>() == 4.0);
    EXPECT_TRUE(tbl["options"]["dotRadius"].value<double>() == 1.5);
    EXPECT_TRUE(tbl["options"]["statsMask"].value<int64_t>() == 7);
    EXPECT_TRUE(tbl["view"]["maxFrequency"].value<int64_t>() == 8000);
}

// A second run changes nothing, and a current file is not touched.
TEST(ConfigMigration, ASecondRunChangesNothingAndACurrentFileIsNotTouched)
{
    toml::table tbl = makeTable(
        "[meta]\nversion = 4\n"
        "[view]\nmaxFrequency = 6000\n"
        "[options]\nlineWidth = 2.5\n");
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["lineWidth"].value<double>() == 2.5);
    EXPECT_TRUE(tbl["view"]["maxFrequency"].value<int64_t>() == 6000);
}

// A new file gets no value for the options, so the defaults of the program apply.
TEST(ConfigMigration, ANewFileGetsNoValueForTheOptionsSoTheDefaultsOfTheProgram)
{
    toml::table tbl = makeTable("");
    migrateConfig(tbl);
    EXPECT_TRUE(!tbl["options"]["lineWidth"]);
    EXPECT_TRUE(tbl["analysis"]["granularity"].value<double>() == 10.0);
    EXPECT_TRUE(tbl["view"]["timeSpan"].value<double>() == 15.0);
    EXPECT_TRUE(tbl["meta"]["version"].value<int64_t>() == kConfigVersion);
}

// The old style of all tracks becomes a style for each track.
TEST(ConfigMigration, TheOldStyleOfAllTracksBecomesAStyleForEachTrack)
{
    toml::table tbl = makeTable("[meta]\nversion = 4\n[options]\ntrackStyle = 1\ntrackStyleF2 = 3\n");
    migrateConfig(tbl);
    EXPECT_TRUE(!tbl["options"]["trackStyle"]);
    EXPECT_TRUE(tbl["options"]["trackStylePitch"].value<int64_t>() == 2);
    EXPECT_TRUE(tbl["options"]["trackStyleF1"].value<int64_t>() == 2);
    EXPECT_TRUE(tbl["options"]["trackStyleF2"].value<int64_t>() == 3);
    EXPECT_TRUE(tbl["options"]["trackStyleF4"].value<int64_t>() == 2);
}

TEST(ConfigMigration, Block)
{
    toml::table tbl = makeTable("[meta]\nversion = 4\n[options]\ntrackStyle = 0\n");
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["trackStylePitch"].value<int64_t>() == 1);
}

TEST(ConfigMigration, Block2)
{
    toml::table tbl = makeTable("[meta]\nversion = 4\n[options]\ntrackStyle = 2\n");
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["trackStylePitch"].value<int64_t>() == 3);
}

// The statistics boxes show by default. A file with the old default "off" turns them on one time.
TEST(ConfigMigration, TheStatisticsBoxesShowByDefaultAFileWithTheOldDefaultOff)
{
    toml::table tbl = makeTable("[meta]\nversion = 5\n[options]\nshowStats = false\n");
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["showStats"].value<bool>() == true);
    // The user turns them off after the migration. The next start keeps the choice.
    tbl["options"].as_table()->insert_or_assign("showStats", false);
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["showStats"].value<bool>() == false);
}

// A file with no value for the statistics keeps no value, so the default of the program applies.
TEST(ConfigMigration, AFileWithNoValueForTheStatisticsKeepsNoValueSoTheDefaultOf)
{
    toml::table tbl = makeTable("[meta]\nversion = 5\n");
    migrateConfig(tbl);
    EXPECT_TRUE(!tbl["options"]["showStats"]);
}

// The boxes stand at the left by default. A file with the old default "bottom" changes one time.
TEST(ConfigMigration, TheBoxesStandAtTheLeftByDefaultAFileWithTheOldDefaultBottom)
{
    toml::table tbl = makeTable("[meta]\nversion = 6\n[options]\nreadoutSide = \"bottom\"\n");
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["readoutSide"].value<std::string>() == "left");
    // The user chooses the row after the migration. The next start keeps the choice.
    tbl["options"].as_table()->insert_or_assign("readoutSide", "bottom");
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["readoutSide"].value<std::string>() == "bottom");
}

// A file with no value keeps no value, so the default of the program applies.
TEST(ConfigMigration, AFileWithNoValueKeepsNoValueSoTheDefaultOfTheProgramApplies)
{
    toml::table tbl = makeTable("[meta]\nversion = 6\n");
    migrateConfig(tbl);
    EXPECT_TRUE(!tbl["options"]["readoutSide"]);
}

// The old switch for all formant tracks becomes one switch for each track.
TEST(ConfigMigration, TheOldSwitchForAllFormantTracksBecomesOneSwitchForEachTrack)
{
    toml::table tbl = makeTable("[meta]\nversion = 7\n[view]\nshowFormants = false\n");
    migrateConfig(tbl);
    for (const char *key : { "showTrackF1", "showTrackF2", "showTrackF3", "showTrackF4" }) {
        EXPECT_TRUE(tbl["options"][key].value<bool>() == false);
    }
    // The user turns F2 on after the migration. The next start keeps the choice.
    tbl["options"].as_table()->insert_or_assign("showTrackF2", true);
    migrateConfig(tbl);
    EXPECT_TRUE(tbl["options"]["showTrackF2"].value<bool>() == true);
}

TEST(ConfigMigration, Block3)
{
    toml::table tbl = makeTable("[meta]\nversion = 7\n[view]\nshowFormants = true\n");
    migrateConfig(tbl);
    EXPECT_TRUE(!tbl["options"]["showTrackF1"]);
}
