#include <gtest/gtest.h>

#include <QVariantMap>
#include <cmath>
#include <set>

#include "../src/context/config.h"
#include "../src/context/sessionfile.h"
#include "../src/gui/statformat.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <sstream>

// Print a QString as text in the report of a failed check.
void PrintTo(const QString &text, std::ostream *os)
{
    *os << '"' << text.toStdString() << '"';
}

using namespace Main;

// The audio code is not part of this test program. The configuration asks for the default backend
// only when the file has no value, and the tests never use that value.
namespace Main {
Module::Audio::Backend getDefaultAudioBackend()
{
    return Module::Audio::Backend::Dummy;
}
}

namespace {

toml::table parseTable(const char *text)
{
    return toml::parse(text);
}

// Give every option and every view value a value that is not the default.
void scramble(Config &c)
{
    const QVariantMap defaults = Config::defaultOptions();
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        const int type = it.value().typeId();
        if (type == QMetaType::Bool) c.setOption(it.key(), !it.value().toBool());
        else if (type == QMetaType::Int) c.setOption(it.key(), it.value().toInt() + 1);
        else if (type == QMetaType::Double) c.setOption(it.key(), it.value().toDouble() + 1.0);
        else c.setOption(it.key(), it.value().toString() + "x");
    }

    c.setViewZoom(2.5);
    c.setViewMinFrequency(50);
    c.setViewMaxFrequency(8000);
    c.setViewFFTSize(4096);
    c.setViewMaxGain(12);
    c.setViewSpectrogramQuality(0);
    c.setViewSpectrogramBlur(80);
    c.setViewTimeSpan(30.0);
    c.setViewFrequencyScale(FrequencyScale::Linear);
    c.setViewShowSpectrogram(false);
    c.setViewShowPitch(false);
    c.setPitchAlgorithm(PitchAlgorithm::Yin);
    c.setLinpredAlgorithm(LinpredAlgorithm::Autocorr);
    c.setFormantAlgorithm(FormantAlgorithm::Simple);
    c.setInvglotAlgorithm(InvglotAlgorithm::IAIF);
    c.setAudioCaptureDevice("Some microphone");
}

}

// A reset gives every option its default value. The folder of the practice sessions stays.
namespace {

// Set HOME for one test, and restore it at the end of the scope, also after a failed assertion.
class HomeGuard {
public:
    explicit HomeGuard(const QString &path) : mOld(qgetenv("HOME")), mHadHome(qEnvironmentVariableIsSet("HOME"))
    {
        qputenv("HOME", path.toUtf8());
    }
    ~HomeGuard()
    {
        if (mHadHome) qputenv("HOME", mOld);
        else qunsetenv("HOME");
    }

private:
    QByteArray mOld;
    bool mHadHome;
};

}

TEST(ConfigReset, TP_RST_02_ResetGivesEveryOptionItsDefaultExceptThePracticeFolder)
{
    Config c{toml::table{}};
    scramble(c);
    ASSERT_NE(c.optionString("practiceFolder"), "");

    c.resetAll();

    const QVariantMap defaults = Config::defaultOptions();
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        if (it.key() == "practiceFolder") continue;
        EXPECT_EQ(c.option(it.key()), it.value()) << it.key().toStdString();
    }
    EXPECT_EQ(c.optionString("practiceFolder"), Config::defaultOptions().value("practiceFolder").toString() + "x");
}

// A reset gives the view values their defaults.
TEST(ConfigReset, TP_RST_03_ResetGivesTheViewValuesTheirDefaults)
{
    Config c{toml::table{}};
    scramble(c);

    c.resetAll();

    EXPECT_EQ(c.getViewZoom(), 1.0);
    EXPECT_EQ(c.getViewMinFrequency(), 1);
    EXPECT_EQ(c.getViewMaxFrequency(), 5500);
    EXPECT_EQ(c.getViewFFTSize(), 2048);
    EXPECT_EQ(c.getViewMaxGain(), 0);
    EXPECT_EQ(c.getViewSpectrogramQuality(), 1);
    EXPECT_EQ(c.getViewSpectrogramBlur(), 33);
    EXPECT_EQ(c.getViewTimeSpan(), 15.0);
    EXPECT_EQ(c.getViewFrequencyScale(), FrequencyScale::ERB);
    EXPECT_TRUE(c.getViewShowSpectrogram());
    EXPECT_TRUE(c.getViewShowPitch());
    EXPECT_EQ(c.getAudioCaptureDevice(), "");
}

// A reset gives the four algorithms their defaults.
TEST(ConfigReset, TP_RST_04_ResetGivesTheFourAlgorithmsTheirDefaults)
{
    Config c{toml::table{}};
    scramble(c);

    c.resetAll();

    EXPECT_EQ(c.getPitchAlgorithm(), PitchAlgorithm::RAPT);
    EXPECT_EQ(c.getLinpredAlgorithm(), LinpredAlgorithm::Burg);
    EXPECT_EQ(c.getFormantAlgorithm(), FormantAlgorithm::Filtered);
    EXPECT_EQ(c.getInvglotAlgorithm(), InvglotAlgorithm::GFM_IAIF);
}

// A second reset changes nothing and sends no signal.
TEST(ConfigReset, TP_RST_10_ASecondResetChangesNothing)
{
    Config c{toml::table{}};
    scramble(c);
    c.resetAll();
    const QVariantMap after = c.options();

    int changes = 0;
    QObject::connect(&c, &Config::optionsChanged, [&] { ++changes; });
    c.resetAll();

    EXPECT_EQ(c.options(), after);
    EXPECT_EQ(changes, 0);
    EXPECT_EQ(c.getViewMaxFrequency(), 5500);
    EXPECT_EQ(c.getPitchAlgorithm(), PitchAlgorithm::RAPT);
}

// A Config on a table in memory never writes the configuration file.
TEST(ConfigReset, TP_RST_15_AConfigOnATableInMemoryNeverWritesTheFile)
{
    QTemporaryDir home;
    ASSERT_TRUE(home.isValid());
    HomeGuard homeGuard(home.path());

    // The path of the file follows the home folder on this system. Skip the test when it does not.
    const QString path = QString::fromStdString(getConfigPath().string());
    if (!path.startsWith(home.path())) {
        GTEST_SKIP() << "The configuration path does not follow HOME: " << path.toStdString();
    }

    {
        Config c{toml::table{}};
        c.setOption("lineWidth", 5.0);
        c.save();
    }

    EXPECT_FALSE(QFile::exists(path));
    EXPECT_FALSE(QFile::exists(QFileInfo(path).absolutePath()));
}

// A new file has the statistics boxes on, with the default statistics.
TEST(ConfigDefaults, TP_SET_07_ANewFileShowsTheStatisticsWithTheDefaultValues)
{
    Config c{toml::table{}};

    EXPECT_TRUE(c.optionBool("showStats"));
    // Current, minimum, mean, median, P95, and maximum.
    EXPECT_EQ(c.optionInt("statsMask"), 0x1 | 0x2 | 0x8 | 0x10 | 0x40 | 0x80);
    // The boxes of the pitch and of the four formants show.
    EXPECT_TRUE(c.optionBool("showBoxPitch"));
    for (const char *key : { "showBoxF1", "showBoxF2", "showBoxF3", "showBoxF4" }) {
        EXPECT_TRUE(c.optionBool(key)) << key;
    }
    EXPECT_EQ(c.optionString("readoutOrder"), "legend,pitch,f1,f2,f3,f4");
}

// A file with an unknown key, a value of the wrong type, and a value in a bad place starts with the defaults.
TEST(ConfigDefaults, TP_SET_08_ABrokenFileStartsWithTheDefaultsForTheBrokenParts)
{
    Config c{parseTable(
        "unknownTopKey = 1\n"
        "[options]\n"
        "madeUpOption = \"x\"\n"
        "lineWidth = \"thick\"\n"      // a text, not a number
        "showStats = 3\n"              // a number, not a switch
        "dotRadius = 4.5\n"            // a good value stays
        "[view]\n"
        "maxFrequency = \"high\"\n"
        "fftSize = 4096\n")};

    EXPECT_EQ(c.optionDouble("lineWidth"), 2.0);
    EXPECT_TRUE(c.optionBool("showStats"));
    EXPECT_EQ(c.optionDouble("dotRadius"), 4.5);
    EXPECT_FALSE(c.options().contains("madeUpOption"));
    EXPECT_EQ(c.getViewMaxFrequency(), 5500);
    EXPECT_EQ(c.getViewFFTSize(), 4096);
}

// A file with a syntax error gives an empty table, and so the defaults. The error text names the problem.
TEST(ConfigDefaults, TP_SET_08_AFileWithABrokenLineGivesTheDefaults)
{
    std::istringstream broken("[options]\nlineWidth = 3.0\nthis line is not valid\n[view]\nfftSize = 4096\n");
    std::string error;
    toml::table tbl = parseConfigStream(broken, "informant.conf", &error);
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(tbl.empty());

    Config c{std::move(tbl)};
    EXPECT_EQ(c.optionDouble("lineWidth"), 2.0);
    EXPECT_EQ(c.getViewFFTSize(), 2048);

    // A good file gives no error and keeps its values.
    std::istringstream good("[options]\nlineWidth = 3.0\n");
    error.clear();
    toml::table kept = parseConfigStream(good, "informant.conf", &error);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(Config{std::move(kept)}.optionDouble("lineWidth"), 3.0);
}

// A broken file stays on the disk as informant.conf.broken, and the program starts with the defaults.
TEST(ConfigDefaults, TP_SET_18_ABrokenFileIsKeptAsABackupAndTheTableIsEmpty)
{
    QTemporaryDir home;
    ASSERT_TRUE(home.isValid());
    HomeGuard homeGuard(home.path());

    const QString path = QString::fromStdString(getConfigPath().string());
    if (!path.startsWith(home.path())) {
        GTEST_SKIP() << "The configuration path does not follow HOME: " << path.toStdString();
    }

    ASSERT_TRUE(QDir().mkpath(QFileInfo(path).absolutePath()));
    const QByteArray text = "[options]\nlineWidth = 3.0\nthis line is not valid\n";
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(text);
    }

    const toml::table tbl = getConfigTable();

    EXPECT_TRUE(tbl.empty());
    QFile backup(path + ".broken");
    ASSERT_TRUE(backup.open(QIODevice::ReadOnly));
    EXPECT_EQ(backup.readAll(), text);
}

// When the backup of a broken file fails, the program must not write the settings. The broken file is the only copy.
TEST(ConfigDefaults, AFailedBackupKeepsTheBrokenFileOnTheDisk)
{
    QTemporaryDir home;
    ASSERT_TRUE(home.isValid());
    HomeGuard homeGuard(home.path());

    const QString path = QString::fromStdString(getConfigPath().string());
    if (!path.startsWith(home.path())) GTEST_SKIP() << "The configuration path does not follow HOME: " << path.toStdString();

    ASSERT_TRUE(QDir().mkpath(QFileInfo(path).absolutePath()));
    const QByteArray text = "[options]\nlineWidth = 3.0\nthis line is not valid\n";
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(text);
    }
    // A folder with the name of the backup makes the copy fail.
    ASSERT_TRUE(QDir().mkpath(path + ".broken"));

    {
        Config config;
        config.save();
    }
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(file.readAll(), text);
}

// A normal save writes the file in one step and leaves no temporary file.
TEST(ConfigDefaults, ASaveLeavesTheFileAndNoTemporaryFile)
{
    QTemporaryDir home;
    ASSERT_TRUE(home.isValid());
    HomeGuard homeGuard(home.path());

    const QString path = QString::fromStdString(getConfigPath().string());
    if (!path.startsWith(home.path())) GTEST_SKIP() << "The configuration path does not follow HOME: " << path.toStdString();

    {
        Config config;
        config.save();
    }
    EXPECT_TRUE(QFileInfo::exists(path));
    EXPECT_FALSE(QFileInfo::exists(path + ".tmp"));
    EXPECT_GT(QFileInfo(path).size(), 0);
}

// A sub-table with a value of the wrong kind (a number in place of a table) gives the defaults.
TEST(ConfigDefaults, TP_SET_08_ASubTableOfTheWrongKindGivesTheDefaults)
{
    Config c{parseTable("options = 5\nview = \"text\"\naudio = [1, 2]\nanalysis = true\n")};

    EXPECT_EQ(c.optionDouble("lineWidth"), 2.0);
    EXPECT_EQ(c.getViewMaxFrequency(), 5500);
    EXPECT_EQ(c.getAudioCaptureDevice(), "");
    EXPECT_EQ(c.getAnalysisGranularity(), 10.0);
}

// The boxes stand at the left of the graph when the file has no choice.
TEST(ConfigDefaults, TP_SET_15_ANewFileHasTheBoxesAtTheLeft)
{
    Config c{toml::table{}};
    EXPECT_EQ(c.optionString("readoutSide"), "left");
}

// The flip of the time axis is on in a new file and in a file with no value. A stored false stays.
TEST(ConfigDefaults, TP_FLIP_16_TheFlipIsOnByDefaultAndAStoredFalseStays)
{
    Config fresh{toml::table{}};
    EXPECT_TRUE(fresh.optionBool("flipTime"));

    Config old{parseTable("[meta]\nversion = 2\n[options]\nlineWidth = 3.0\n")};
    EXPECT_TRUE(old.optionBool("flipTime"));

    Config stored{parseTable("[options]\nflipTime = false\n")};
    EXPECT_FALSE(stored.optionBool("flipTime"));
}

// Every setter works when the file has no key for it, and when the whole sub-table is missing.
TEST(ConfigSetters, TP_SET_16_EverySetterWorksWhenTheFileHasNoKey)
{
    Config c{toml::table{}};

    c.setAudioCaptureDevice("Mic A");
    EXPECT_EQ(c.getAudioCaptureDevice(), "Mic A");

    c.setPitchAlgorithm(PitchAlgorithm::MPM);
    EXPECT_EQ(c.getPitchAlgorithm(), PitchAlgorithm::MPM);
    c.setPitchAlgorithm((int) PitchAlgorithm::Yin);
    EXPECT_EQ(c.getPitchAlgorithmNumeric(), (int) PitchAlgorithm::Yin);

    c.setLinpredAlgorithm(LinpredAlgorithm::Covar);
    EXPECT_EQ(c.getLinpredAlgorithm(), LinpredAlgorithm::Covar);
    c.setLinpredAlgorithm((int) LinpredAlgorithm::Autocorr);
    EXPECT_EQ(c.getLinpredAlgorithmNumeric(), (int) LinpredAlgorithm::Autocorr);

    c.setFormantAlgorithm(FormantAlgorithm::Simple);
    EXPECT_EQ(c.getFormantAlgorithm(), FormantAlgorithm::Simple);
    c.setFormantAlgorithm((int) FormantAlgorithm::Filtered);
    EXPECT_EQ(c.getFormantAlgorithmNumeric(), (int) FormantAlgorithm::Filtered);

    c.setInvglotAlgorithm(InvglotAlgorithm::IAIF);
    EXPECT_EQ(c.getInvglotAlgorithm(), InvglotAlgorithm::IAIF);
    c.setInvglotAlgorithm((int) InvglotAlgorithm::GFM_IAIF);
    EXPECT_EQ(c.getInvglotAlgorithmNumeric(), (int) InvglotAlgorithm::GFM_IAIF);

    c.setViewZoom(3.0);
    EXPECT_EQ(c.getViewZoom(), 3.0);
    c.setViewMinFrequency(20);
    EXPECT_EQ(c.getViewMinFrequency(), 20);
    c.setViewMaxFrequency(7000);
    EXPECT_EQ(c.getViewMaxFrequency(), 7000);
    c.setViewFFTSize(1024);
    EXPECT_EQ(c.getViewFFTSize(), 1024);
    c.setViewMaxGain(-6);
    EXPECT_EQ(c.getViewMaxGain(), -6);
    c.setViewSpectrogramQuality(0);
    EXPECT_EQ(c.getViewSpectrogramQuality(), 0);
    c.setViewSpectrogramBlur(70);
    EXPECT_EQ(c.getViewSpectrogramBlur(), 70);
    c.setViewTimeSpan(42.0);
    EXPECT_EQ(c.getViewTimeSpan(), 42.0);
    c.setViewFrequencyScale(FrequencyScale::Mel);
    EXPECT_EQ(c.getViewFrequencyScale(), FrequencyScale::Mel);
    c.setViewFrequencyScale((int) FrequencyScale::Logarithmic);
    EXPECT_EQ(c.getViewFrequencyScaleNumeric(), (int) FrequencyScale::Logarithmic);
    c.setViewShowSpectrogram(false);
    EXPECT_FALSE(c.getViewShowSpectrogram());
    c.setViewShowPitch(false);
    EXPECT_FALSE(c.getViewShowPitch());

    c.setAudioBackend(Module::Audio::Backend::PortAudio);
    EXPECT_EQ(c.getAudioBackend(), Module::Audio::Backend::PortAudio);

    c.setAnalysisGranularity(11.0);
    EXPECT_EQ(c.getAnalysisGranularity(), 11.0);
    c.setAnalysisSpectrogramWindow(51.0);
    EXPECT_EQ(c.getAnalysisSpectrogramWindow(), 51.0);
    c.setAnalysisPitchWindow(41.0);
    EXPECT_EQ(c.getAnalysisPitchWindow(), 41.0);
    c.setAnalysisFormantWindow(21.0);
    EXPECT_EQ(c.getAnalysisFormantWindow(), 21.0);
    c.setAnalysisOscilloscopeWindow(81.0);
    EXPECT_EQ(c.getAnalysisOscilloscopeWindow(), 81.0);
    c.setAnalysisPitchSpacing(22.0);
    EXPECT_EQ(c.getAnalysisPitchSpacing(), 22.0);
    c.setAnalysisFormantSpacing(42.0);
    EXPECT_EQ(c.getAnalysisFormantSpacing(), 42.0);
    c.setAnalysisOscilloscopeSpacing(23.0);
    EXPECT_EQ(c.getAnalysisOscilloscopeSpacing(), 23.0);
}

// The setters work for a file that has the sub-tables with a wrong kind of value, and for an unknown option.
TEST(ConfigSetters, TP_SET_16_ASetterWorksOnATableOfTheWrongKindAndAnUnknownOptionIsIgnored)
{
    Config c{parseTable("view = 1\nsolvers = \"x\"\nanalysis = []\naudio = 0\n")};

    c.setViewMaxFrequency(6000);
    c.setPitchAlgorithm(PitchAlgorithm::Yin);
    c.setAnalysisGranularity(12.0);
    c.setAudioCaptureDevice("Mic");
    EXPECT_EQ(c.getViewMaxFrequency(), 6000);
    EXPECT_EQ(c.getPitchAlgorithm(), PitchAlgorithm::Yin);
    EXPECT_EQ(c.getAnalysisGranularity(), 12.0);
    EXPECT_EQ(c.getAudioCaptureDevice(), "Mic");

    c.setOption("noSuchOption", 1);
    EXPECT_FALSE(c.options().contains("noSuchOption"));
}

// The default of each value outside the option map comes from one place. A getter with no value and the reset agree.
TEST(ConfigSetters, TP_SET_17_TheGetterDefaultAndTheResetAgree)
{
    Config fresh{toml::table{}};
    Config reset{toml::table{}};
    scramble(reset);
    reset.resetAll();

    EXPECT_EQ(fresh.getViewZoom(), reset.getViewZoom());
    EXPECT_EQ(fresh.getViewMinFrequency(), reset.getViewMinFrequency());
    EXPECT_EQ(fresh.getViewMaxFrequency(), reset.getViewMaxFrequency());
    EXPECT_EQ(fresh.getViewFFTSize(), reset.getViewFFTSize());
    EXPECT_EQ(fresh.getViewMaxGain(), reset.getViewMaxGain());
    EXPECT_EQ(fresh.getViewSpectrogramQuality(), reset.getViewSpectrogramQuality());
    EXPECT_EQ(fresh.getViewSpectrogramBlur(), reset.getViewSpectrogramBlur());
    EXPECT_EQ(fresh.getViewTimeSpan(), reset.getViewTimeSpan());
    EXPECT_EQ(fresh.getViewFrequencyScale(), reset.getViewFrequencyScale());
    EXPECT_EQ(fresh.getViewShowSpectrogram(), reset.getViewShowSpectrogram());
    EXPECT_EQ(fresh.getViewShowPitch(), reset.getViewShowPitch());
    EXPECT_EQ(fresh.getPitchAlgorithm(), reset.getPitchAlgorithm());
    EXPECT_EQ(fresh.getLinpredAlgorithm(), reset.getLinpredAlgorithm());
    EXPECT_EQ(fresh.getFormantAlgorithm(), reset.getFormantAlgorithm());
    EXPECT_EQ(fresh.getInvglotAlgorithm(), reset.getInvglotAlgorithm());
    EXPECT_EQ(fresh.options(), Config::defaultOptions());
}

// The display has one switch for each of four formant tracks. A file with another count never reads past them.
TEST(ConfigTracks, TP_TRACK_08_TheFormantCountStaysInTheRangeOfTheFourSwitches)
{
    EXPECT_EQ(Config::kFormantTracks, 4);

    Config none{toml::table{}};
    EXPECT_EQ(none.getViewFormantCount(), 4);

    Config three{parseTable("[view]\nformantCount = 3\n")};
    EXPECT_EQ(three.getViewFormantCount(), 3);

    Config five{parseTable("[view]\nformantCount = 5\n")};
    EXPECT_EQ(five.getViewFormantCount(), 4);

    Config negative{parseTable("[view]\nformantCount = -2\n")};
    EXPECT_EQ(negative.getViewFormantCount(), 0);

    Config huge{parseTable("[view]\nformantCount = 9000000000\n")};
    EXPECT_EQ(huge.getViewFormantCount(), 4);
}

// Nothing reads the old switch for all formant tracks. The migration turns it into four switches.
TEST(ConfigTracks, TP_TRACK_03_TheOldFormantSwitchIsOnlyReadByTheMigration)
{
    Config c{parseTable("[meta]\nversion = 7\n[view]\nshowFormants = false\n")};
    for (const char *key : { "showTrackF1", "showTrackF2", "showTrackF3", "showTrackF4" }) {
        EXPECT_FALSE(c.optionBool(key)) << key;
    }

    // The Config class has no property for the old switch.
    EXPECT_EQ(Config::staticMetaObject.indexOfProperty("viewShowFormants"), -1);
}

// The Range in semitones is 12 times log2(P95 / P5). A P5 of 0 or less gives a dash.
TEST(RangeText, TP_SET_13_TheRangeInSemitonesIsTwelveTimesTheLogOfTheRatio)
{
    // One octave is 12 semitones, and two octaves are 24.
    EXPECT_EQ(Gui::rangeSemitonesText(100.0, 200.0), "12.0st");
    EXPECT_EQ(Gui::rangeSemitonesText(100.0, 400.0), "24.0st");
    EXPECT_EQ(Gui::rangeSemitonesText(100.0, 100.0), "0.0st");
    EXPECT_EQ(Gui::rangeSemitonesText(110.0, 123.47), "2.0st");

    EXPECT_EQ(Gui::rangeSemitonesText(0.0, 200.0), "-");
    EXPECT_EQ(Gui::rangeSemitonesText(-5.0, 200.0), "-");
    EXPECT_EQ(Gui::rangeSemitonesText(200.0, 100.0), "-");
    EXPECT_EQ(Gui::rangeSemitonesText(std::nan(""), 100.0), "-");
}

// The name of the scratch file of a session has the process ID.
TEST(SessionFile, TP_CAP_08_TheScratchFileNameHasTheProcessId)
{
    const QString name = sessionFileTemplate(4242);
    EXPECT_EQ(name, "informant-session-4242-XXXXXX.raw");

    // The name that QTemporaryFile makes keeps the process ID.
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QTemporaryFile file(dir.path() + "/" + name);
    ASSERT_TRUE(file.open());
    qint64 pid = 0;
    EXPECT_TRUE(sessionFilePid(QFileInfo(file.fileName()).completeBaseName(), pid));
    EXPECT_EQ(pid, 4242);

    // A name of an older version has no process ID.
    EXPECT_FALSE(sessionFilePid("informant-session-AbCdEf", pid));
    EXPECT_FALSE(sessionFilePid("informant-session-notanumber-AbCdEf", pid));
}
