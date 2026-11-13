#include "contextmanager.h"
#include "config.h"
#include "configmigration.h"
#include <QFile>
#include <cmath>
#include "cfgpath.h"
#include "../gui/theme.h"
#include <algorithm>
#include <iostream>

using namespace Main;

namespace {
// The default values of the settings that live outside the option map. A getter and the reset function read the same value.
constexpr double kDefaultZoom = 1.0;
constexpr int kDefaultMinFrequency = 1;
constexpr int kDefaultMaxFrequency = 5500;
constexpr int kDefaultFftSize = 2048;
constexpr int kDefaultMaxGain = 0;
constexpr int kDefaultSpectrogramQuality = 1;
constexpr int kDefaultSpectrogramBlur = 33;
constexpr double kDefaultTimeSpan = 15.0;
}

static void initSubTable(toml::table &tbl, const std::string &name)
{
    if (!tbl[name]) {
        tbl.insert(name, toml::table());
    }
    else if (tbl[name].type() != toml::node_type::table) {
        tbl.insert_or_assign(name, toml::table());
    }
}

template<typename E>
constexpr auto enumInt(const E &e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

template<typename T, typename V, typename ViewedType>
constexpr T valueCastField(toml::node_view<ViewedType> tblNode, const std::string &name, T defVal) {
    auto node = tblNode[name];
    auto opt = node.template value<V>();
    if (opt) {
        return static_cast<T>(*opt);
    }
    else {
        tblNode.as_table()->insert(name, static_cast<V>(defVal));
        return defVal;
    }
}

template<typename T, typename ViewedType>
constexpr T valueField(toml::node_view<ViewedType> tblNode, const std::string &name, T defVal) {
    auto node = tblNode[name];
    auto opt = node.template value<T>();
    if (opt) {
        return static_cast<T>(*opt);
    }
    else {
        tblNode.as_table()->insert(name, static_cast<T>(defVal));
        return defVal;
    }
}

template<typename T, typename ViewedType>
constexpr T enumField(toml::node_view<ViewedType> tblNode, const std::string &name, T defVal) {
    return valueCastField<T, int64_t, ViewedType>(tblNode, name, defVal);
}

template<typename ViewedType>
constexpr int64_t integerField(toml::node_view<ViewedType> tblNode, const std::string &name, int64_t defVal) {
    return valueField<int64_t, ViewedType>(tblNode, name, defVal);
}

template<typename ViewedType>
constexpr double doubleField(toml::node_view<ViewedType> tblNode, const std::string &name, double defVal) {
    return valueField<double, ViewedType>(tblNode, name, defVal);
}

template<typename ViewedType>
constexpr bool boolField(toml::node_view<ViewedType> tblNode, const std::string &name, bool defVal) {
    return valueField<bool, ViewedType>(tblNode, name, defVal);
}

fs::path Main::getConfigPath()
{
    cfgpathchar_t cfgdir[MAX_PATH];
    get_user_config_file(cfgdir, sizeof(cfgdir) / sizeof(cfgdir[0]), CFGPATHTEXT("informant"));
    if (cfgdir[0] == 0) {
        throw std::runtime_error("ContextManager] Unable to find config path.");
    }
    return fs::path(cfgdir);
}

toml::table Main::parseConfigStream(std::istream &stream, const std::string &source, std::string *error)
{
    try {
        return toml::parse(stream, source);
    }
    catch (const toml::parse_error &e) {
        if (error) *error = e.what();
        return toml::table();
    }
}

namespace {
// True when the last read found a broken file and could not keep a copy of it. The program then must not
// write the file, because the broken file would be lost.
bool gBackupFailed = false;
}

toml::table Main::getConfigTable()
{
    gBackupFailed = false;
    auto path = getConfigPath();

#if defined(_WIN32) && defined(UNICODE)
    std::wcout << "Reading configuration from: " << path << std::endl;
#else
    std::cout << "Reading configuration from: " << path << std::endl;
#endif

    fs::create_directories(path.parent_path());

    if (fs::exists(path) && fs::is_regular_file(path)) {
        std::ifstream ifs(path, std::ios_base::binary);
        std::string error;
        toml::table tbl = parseConfigStream(ifs, path.string(), &error);
        if (!error.empty()) {
            // Keep the broken file next to the new one, so the user can repair it.
            std::cout << "Config is broken, using defaults: " << error << std::endl;
            ifs.close();
            fs::path backup = path;
            backup += ".broken";
            std::error_code ec;
            fs::copy_file(path, backup, fs::copy_options::overwrite_existing, ec);
            if (ec) gBackupFailed = true;
        }
        return tbl;
    }

    std::cout << "Config not found, using defaults..." << std::endl;
    return toml::table();
}

Config::Config()
    : Config(getConfigTable(), true)
{
}

Config::Config(toml::table table)
    : Config(std::move(table), false)
{
}

Config::Config(toml::table table, bool persist)
    : mTbl(std::move(table)),
      mPersist(persist && !gBackupFailed),
      mPaused(true),
      mViewLive(true),
      mViewEnd(0.0),
      mCursorTime(std::nan("")),
      mCursorClickX(-1.0),
      mCursorClickY(-1.0),
      mSessionMode(0)
{
    initSubTable(mTbl, "audio");
    initSubTable(mTbl, "solvers");
    initSubTable(mTbl, "view");
    initSubTable(mTbl, "ui");
    initSubTable(mTbl, "analysis");
    initSubTable(mTbl, "options");
    initSubTable(mTbl, "meta");

    migrateConfig(mTbl);

    // Read the options. An option with no stored value, or with a value of the wrong type, keeps its default.
    mOptions = defaultOptions();
    for (auto it = mOptions.begin(); it != mOptions.end(); ++it) {
        auto node = mTbl["options"][it.key().toStdString()];
        const int type = it.value().typeId();
        if (type == QMetaType::Bool) {
            if (auto v = node.value<bool>()) it.value() = *v;
        }
        else if (type == QMetaType::Int) {
            if (auto v = node.value<int64_t>()) it.value() = (int) *v;
        }
        else if (type == QMetaType::Double) {
            if (auto v = node.value<double>()) it.value() = *v;
        }
        else if (type == QMetaType::QString) {
            if (auto v = node.value<std::string>()) it.value() = QString::fromStdString(*v);
        }
    }
}

QVariantMap Config::defaultOptions()
{
    QVariantMap m;

    // Readouts and overlays.
    m["showDebug"] = false;
    m["showLegend"] = true;
    m["showStats"] = true;
    m["showMiniMap"] = true;
    m["pitchAxisMin"] = 0.0;     // Hz: the low edge of the pitch-only graph
    m["pitchAxisMax"] = 500.0;   // Hz: the high edge of the pitch-only graph
    m["pitchAxisAuto"] = false;  // fit the pitch-only axis to the pitch in view
    m["practiceTag"] = QString("Free");      // the task of the next practice session
    m["practiceFolder"] = QString("");       // the folder of the sessions, empty for Documents/InFormant/Practice
    m["practiceKeepAudio"] = true;           // write the sound of a session into its folder
    m["practiceMinSeconds"] = 5;             // a session shorter than this time is dropped
    m["graphMode"] = QString("full");  // "full" for the spectrogram and all tracks, "pitch" for the pitch track alone
    m["rangeSemitones"] = false; // show the Range statistic in semitones and not in Hz
    m["flipTime"] = true;        // the newest sound is at the left edge and the graph scrolls to the right
    m["gridStrength"] = 0.6;     // the opacity of the grid lines from 0.1 to 1
    m["freqTickDensity"] = 1;    // labels of the automatic frequency axis: 0 sparse, 1 normal, 2 dense
    m["gridMajor"] = false;      // draw a line across the graph at each labeled tick of the frequency axis
    m["gridMinor"] = false;      // draw a fainter line at each tick without a label
    m["showBands"] = false;      // show the pitch zones in the graph
    m["bandsInExport"] = true;   // keep the zones on the screen while a recording runs
    m["bandEdges"] = QString("60,150,180,300");  // Hz: the edges of the three zones
    m["mapWindow"] = 150.0;      // seconds of the session that the map shows, 150 s is 2.5 minutes

    // The readout boxes. The order lists the keys. The side is "bottom" or "left". One switch for each track box.
    m["readoutOrder"] = QString("legend,pitch,f1,f2,f3,f4");
    m["readoutSide"] = QString("left");
    m["showBoxPitch"] = true;
    m["showTrackF1"] = true;     // draw the track of the formant F1 in the graph
    m["showTrackF2"] = true;
    m["showTrackF3"] = true;
    m["showTrackF4"] = true;
    m["showBoxF1"] = true;
    m["showBoxF2"] = true;
    m["showBoxF3"] = true;
    m["showBoxF4"] = true;

    // Axes.
    m["showAxisLabels"] = true;
    m["showAxisTitles"] = true;
    m["freqTickMode"] = 0;       // 0 automatic, 1 custom step, 2 musical notes, 3 octaves
    m["freqTickStep"] = 100.0;   // Hz, for the custom step
    m["timeTickMode"] = 0;       // 0 automatic, 1 custom step
    m["timeTickStep"] = 1.0;     // seconds, for the custom step

    // Tracks.
    // The style of each track, as bits: 1 dots, 2 line.
    m["trackStylePitch"] = 3;
    m["trackStyleF1"] = 3;
    m["trackStyleF2"] = 3;
    m["trackStyleF3"] = 3;
    m["trackStyleF4"] = 3;
    m["lineSmoothness"] = 35;    // percent
    m["lineWidth"] = 2.0;        // pixels at 100 percent zoom
    m["dotRadius"] = 2.0;        // pixels at 100 percent zoom

    // Track statistics. Each bit of the mask shows one value.
    m["statsMask"] = 0x1 | 0x2 | 0x8 | 0x10 | 0x40 | 0x80;   // current, min, mean, median, p95, max
    m["statsWindow"] = 0.0;      // seconds, 0 means the visible window

    // Oscilloscope.
    m["scopeWindow"] = 30.0;     // milliseconds of sound in the view
    m["scopeShowGif"] = true;    // show the estimate of the glottal flow

    // Recording.
    m["recordWindow"] = 10.0;    // seconds of time in the graph of the video, from 2 to 60
    m["recordFps"] = 20;         // frames each second in the video
    m["recordWindows"] = true;   // show the oscilloscope and the synthesizer in the video
    m["recordVideo"] = true;     // write a video, or only the sound and the report

    // The sound of the capture goes to a scratch file. It is deleted at the end of the run unless this is true.
    m["keepRecordings"] = false;

    // Color theme.
    m["theme"] = QString("Magma");
    m["customColors"] = QString("#000004,#3b0f70,#b73779,#fc8961,#fcfdbf");
    m["customPitch"] = QString("#00e5ff");
    m["customFormants"] = QString("#00ff00,#91ee91,#ff0000,#ff99ff,#ffcc00");
    m["minVolumeBlack"] = false;  // false: the quietest sound has a dark tint of the theme. true: it is black.
    m["colorSaturation"] = 1.0;  // the chroma of the color map, from 0.5 to 1.5 times
    return m;
}

QVariantMap Config::options() const
{
    return mOptions;
}

QVariantMap Config::themeColors() const
{
    const Gui::Theme theme = Gui::resolveTheme(optionString("theme"), optionString("customColors"),
                                               optionString("customPitch"), optionString("customFormants"),
                                               optionDouble("colorSaturation"), optionBool("minVolumeBlack"));
    return Gui::themeColorMap(theme);
}

QStringList Config::themeNames() const
{
    return Gui::themeNames();
}

QVariantList Config::themePreview(const QString &name, int count) const
{
    if (name == "Custom") {
        const Gui::Theme t = Gui::resolveTheme(name, optionString("customColors"), optionString("customPitch"),
                                               optionString("customFormants"), optionDouble("colorSaturation"),
                                               optionBool("minVolumeBlack"));
        QVariantList list;
        for (int i = 0; i < count; ++i) {
            list << QColor::fromRgb(t.colormap[count > 1 ? (i * 255) / (count - 1) : 0]);
        }
        return list;
    }
    return Gui::themePreview(name, count, optionDouble("colorSaturation"), optionBool("minVolumeBlack"));
}

QVariant Config::option(const QString &name) const
{
    return mOptions.value(name);
}

void Config::setOption(const QString &name, const QVariant &value)
{
    const QVariant def = defaultOptions().value(name);
    if (!def.isValid()) {
        return;
    }

    QVariant v = value;
    const int type = def.typeId();
    if (type == QMetaType::Bool) v = value.toBool();
    else if (type == QMetaType::Int) v = value.toInt();
    else if (type == QMetaType::Double) v = value.toDouble();
    else if (type == QMetaType::QString) v = value.toString();

    if (mOptions.value(name) == v) {
        return;
    }
    mOptions[name] = v;

    auto &tbl = *mTbl["options"].as_table();
    const std::string key = name.toStdString();
    if (type == QMetaType::Bool) tbl.insert_or_assign(key, v.toBool());
    else if (type == QMetaType::Int) tbl.insert_or_assign(key, (int64_t) v.toInt());
    else if (type == QMetaType::Double) tbl.insert_or_assign(key, v.toDouble());
    else if (type == QMetaType::QString) tbl.insert_or_assign(key, v.toString().toStdString());

    emit optionsChanged();
}

void Config::resetOption(const QString &name)
{
    setOption(name, defaultOptions().value(name));
}

void Config::resetAll()
{
    // The folder of the practice sessions is a place and not a preference. A reset keeps it, so the sessions stay in the Progress window.
    const QVariantMap defaults = defaultOptions();
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        if (it.key() == "practiceFolder") continue;
        setOption(it.key(), it.value());
    }

    setViewZoom(kDefaultZoom);
    setViewMinFrequency(kDefaultMinFrequency);
    setViewMaxFrequency(kDefaultMaxFrequency);
    setViewFFTSize(kDefaultFftSize);
    setViewMaxGain(kDefaultMaxGain);
    setViewSpectrogramQuality(kDefaultSpectrogramQuality);
    setViewSpectrogramBlur(kDefaultSpectrogramBlur);
    setViewTimeSpan(kDefaultTimeSpan);
    setViewFrequencyScale(FrequencyScale::ERB);
    setViewShowSpectrogram(true);
    setViewShowPitch(true);

    setPitchAlgorithm(PitchAlgorithm::RAPT);
    setLinpredAlgorithm(LinpredAlgorithm::Burg);
    setFormantAlgorithm(FormantAlgorithm::Filtered);
    setInvglotAlgorithm(InvglotAlgorithm::GFM_IAIF);

    // An empty name selects the default input device of the system.
    setAudioCaptureDevice(QString());
}

bool Config::optionBool(const QString &name) const { return mOptions.value(name).toBool(); }
int Config::optionInt(const QString &name) const { return mOptions.value(name).toInt(); }
double Config::optionDouble(const QString &name) const { return mOptions.value(name).toDouble(); }
QString Config::optionString(const QString &name) const { return mOptions.value(name).toString(); }

void Config::save()
{
    if (!mPersist) {
        return;
    }
    // Write to a temporary file, and then rename it. A stop in the middle of the write leaves the old file.
    const fs::path path = getConfigPath();
    fs::path temporary = path;
    temporary += ".tmp";
    std::error_code ec;
    {
        std::ofstream stream(temporary);
        stream << mTbl;
        if (!stream) {
            stream.close();
            fs::remove(temporary, ec);
            return;
        }
    }
    fs::rename(temporary, path, ec);
}

Config::~Config()
{
    save();
}

Module::Audio::Backend Config::getAudioBackend()
{
    return enumField(toml::node_view(mTbl), "audioBackend", Main::getDefaultAudioBackend());
}

void Config::setAudioBackend(Module::Audio::Backend b)
{
    mTbl.insert_or_assign("audioBackend", static_cast<int64_t>(enumInt(b)));
    emit audioBackendChanged(enumInt(b));
}

QString Config::getAudioCaptureDevice()
{
    return QString::fromStdString(valueField<std::string>(mTbl["audio"], "captureDevice", std::string()));
}

void Config::setAudioCaptureDevice(const QString &name)
{
    mTbl["audio"].as_table()->insert_or_assign("captureDevice", name.toStdString());
    emit audioCaptureDeviceChanged(name);
}

QStringList Config::getAudioCaptureDevices() const
{
    return mAudioCaptureDevices;
}

QString Config::getAudioDefaultCaptureDevice() const
{
    return mAudioDefaultCaptureDevice;
}

void Config::setAudioCaptureDevices(const QStringList &names, const QString &defaultName)
{
    mAudioCaptureDevices = names;
    mAudioDefaultCaptureDevice = defaultName;
    emit audioCaptureDevicesChanged();
}

PitchAlgorithm Config::getPitchAlgorithm()
{
    return enumField(mTbl["solvers"], "pitch", PitchAlgorithm::RAPT);
}

void Config::setPitchAlgorithm(PitchAlgorithm alg)
{
    mTbl["solvers"].as_table()->insert_or_assign("pitch", static_cast<int64_t>(enumInt(alg)));
    emit pitchAlgorithmChanged(enumInt(alg));
}

int Config::getPitchAlgorithmNumeric()
{
    return enumInt(getPitchAlgorithm());
}

void Config::setPitchAlgorithm(int alg)
{
    setPitchAlgorithm(static_cast<PitchAlgorithm>(alg));
}

LinpredAlgorithm Config::getLinpredAlgorithm()
{
    return enumField(mTbl["solvers"], "linpred", LinpredAlgorithm::Burg);
}

void Config::setLinpredAlgorithm(LinpredAlgorithm alg)
{
    mTbl["solvers"].as_table()->insert_or_assign("linpred", static_cast<int64_t>(enumInt(alg)));
    emit linpredAlgorithmChanged(enumInt(alg));
}

int Config::getLinpredAlgorithmNumeric()
{
    return enumInt(getLinpredAlgorithm());
}

void Config::setLinpredAlgorithm(int alg)
{
    setLinpredAlgorithm(static_cast<LinpredAlgorithm>(alg));
}

FormantAlgorithm Config::getFormantAlgorithm(bool internal)
{
#ifndef ENABLE_TORCH
    if (!internal && getFormantAlgorithm(true) == FormantAlgorithm::Deep) {
        setFormantAlgorithm(FormantAlgorithm::Filtered);
    }
#endif
    return enumField(mTbl["solvers"], "formant", FormantAlgorithm::Filtered);
}

void Config::setFormantAlgorithm(FormantAlgorithm alg)
{
    mTbl["solvers"].as_table()->insert_or_assign("formant", static_cast<int64_t>(enumInt(alg)));
    emit formantAlgorithmChanged(enumInt(alg));
}

int Config::getFormantAlgorithmNumeric()
{
    return enumInt(getFormantAlgorithm());
}

void Config::setFormantAlgorithm(int alg)
{
    setFormantAlgorithm(static_cast<FormantAlgorithm>(alg));
}

InvglotAlgorithm Config::getInvglotAlgorithm()
{
    return enumField(mTbl["solvers"], "invglot", InvglotAlgorithm::GFM_IAIF);
}

void Config::setInvglotAlgorithm(InvglotAlgorithm alg)
{
    mTbl["solvers"].as_table()->insert_or_assign("invglot", static_cast<int64_t>(enumInt(alg)));
    emit invglotAlgorithmChanged(enumInt(alg));
}

int Config::getInvglotAlgorithmNumeric()
{
    return enumInt(getInvglotAlgorithm());
}

void Config::setInvglotAlgorithm(int alg)
{
    setInvglotAlgorithm(static_cast<InvglotAlgorithm>(alg));
}

double Config::getViewZoom()
{
    return doubleField(mTbl["view"], "zoomScale", kDefaultZoom);
}

void Config::setViewZoom(double scale)
{
    mTbl["view"].as_table()->insert_or_assign("zoomScale", static_cast<double>(scale));
    emit viewZoomChanged(scale);
}

int Config::getViewMinFrequency()
{
    return integerField(mTbl["view"], "minFrequency", kDefaultMinFrequency);
}

void Config::setViewMinFrequency(int f)
{
    mTbl["view"].as_table()->insert_or_assign("minFrequency", static_cast<int64_t>(f));
    emit viewMinFrequencyChanged(f);
}

int Config::getViewMaxFrequency()
{
    return integerField(mTbl["view"], "maxFrequency", kDefaultMaxFrequency);
}

void Config::setViewMaxFrequency(int f)
{
    mTbl["view"].as_table()->insert_or_assign("maxFrequency", static_cast<int64_t>(f));
    emit viewMaxFrequencyChanged(f);
}

int Config::getViewFFTSize()
{
    return integerField(mTbl["view"], "fftSize", kDefaultFftSize);
}

void Config::setViewFFTSize(int nfft)
{
    mTbl["view"].as_table()->insert_or_assign("fftSize", static_cast<int64_t>(nfft));
    emit viewFFTSizeChanged(nfft);
}

int Config::getViewMaxGain()
{
    return integerField(mTbl["view"], "maxGain", kDefaultMaxGain);
}

void Config::setViewMaxGain(int g)
{
    mTbl["view"].as_table()->insert_or_assign("maxGain", static_cast<int64_t>(g));
    emit viewMaxGainChanged(g);
}

int Config::getViewSpectrogramQuality()
{
    return integerField(mTbl["view"], "spectrogramQuality", kDefaultSpectrogramQuality);
}

void Config::setViewSpectrogramQuality(int quality)
{
    mTbl["view"].as_table()->insert_or_assign("spectrogramQuality", static_cast<int64_t>(quality));
    emit viewSpectrogramQualityChanged(quality);
}

int Config::getViewSpectrogramBlur()
{
    return integerField(mTbl["view"], "spectrogramBlur", kDefaultSpectrogramBlur);
}

void Config::setViewSpectrogramBlur(int percent)
{
    mTbl["view"].as_table()->insert_or_assign("spectrogramBlur", static_cast<int64_t>(percent));
    emit viewSpectrogramBlurChanged(percent);
}

double Config::getViewTimeSpan()
{
    return doubleField(mTbl["view"], "timeSpan", kDefaultTimeSpan);
}

void Config::setViewTimeSpan(double dur)
{
    // The span is from half a second to 24 hours.
    dur = std::clamp(dur, 0.5, 86400.0);
    mTbl["view"].as_table()->insert_or_assign("timeSpan", static_cast<double>(dur));
    emit viewTimeSpanChanged(dur);
}

void Config::setViewLive(bool live)
{
    if (mViewLive == live) return;
    mViewLive = live;
    emit viewLiveChanged(live);
}

void Config::setViewEnd(double seconds)
{
    mViewEnd = seconds;
    emit viewEndChanged(seconds);
}

QString Config::helpPage(const QString &name) const
{
    QFile file(":/help/" + name + ".md");
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
}

void Config::pickMoment(double fractionX, double fractionY)
{
    mCursorClickY = fractionY;
    mCursorClickX = fractionX;
}

void Config::clearMoment()
{
    mCursorClickX = -1.0;
    mCursorTime = std::nan("");
}

bool Config::takeCursorClick(double &fractionX, double &fractionY)
{
    fractionX = mCursorClickX.exchange(-1.0);
    if (fractionX < 0.0) return false;
    fractionY = mCursorClickY;
    return true;
}

void Config::setSessionMode(int mode)
{
    if (mSessionMode == mode) return;
    mSessionMode = mode;
    emit sessionModeChanged(mode);
}

FrequencyScale Config::getViewFrequencyScale()
{
    return enumField(mTbl["view"], "frequencyScale", FrequencyScale::ERB);
}

void Config::setViewFrequencyScale(FrequencyScale scale)
{
    mTbl["view"].as_table()->insert_or_assign("frequencyScale", static_cast<int64_t>(enumInt(scale)));
    emit viewFrequencyScaleChanged(enumInt(scale));
}

int Config::getViewFrequencyScaleNumeric()
{
    return enumInt(getViewFrequencyScale());
}

void Config::setViewFrequencyScale(int scale)
{
    setViewFrequencyScale(static_cast<FrequencyScale>(scale));
}

int Config::getViewFormantCount()
{
    const int64_t count = integerField(mTbl["view"], "formantCount", kFormantTracks);
    return (int) std::clamp<int64_t>(count, 0, kFormantTracks);
}

std::tuple<double, double, double> Config::getViewFormantColor(int i)
{
    constexpr double defaultFormantColors[][3] = {
        {0.0,  1.0,  0.0},
        {0.57, 0.93, 0.57},
        {1.0,  0.0,  0.0},
    };

    const double defaultRed   = i < 3 ? defaultFormantColors[i][0] : 1.0;
    const double defaultGreen = i < 3 ? defaultFormantColors[i][1] : 0.6;
    const double defaultBlue  = i < 3 ? defaultFormantColors[i][2] : 1.0;

    auto formantColors = mTbl["view"]["formantColors"];
    
    if (!!formantColors && formantColors.is_array()) {
        auto& array = *formantColors.as_array();

        if (i < array.size() && array[i].is_array()) {
            auto& color = *array[i].as_array();
            color.resize(3, 1.0);

            auto colorIt = color.begin();
            double r, g, b;

            r = (colorIt++)->value_or(defaultRed);
            g = (colorIt++)->value_or(defaultGreen);
            b = (colorIt++)->value_or(defaultBlue);

            return {
                r, g, b
            };
        }
        else {
            array.resize(i + 1, toml::array{defaultRed, defaultGreen, defaultBlue});
        }
    }
    else {
        mTbl["view"].as_table()->insert_or_assign("formantColors", toml::array{
            toml::array{0.0,  1.0,  0.0},
            toml::array{0.57, 0.93, 0.57},
            toml::array{1.0,  0.0,  0.0},
        });
    }

    return getViewFormantColor(i);
}

bool Config::getViewShowSpectrogram()
{
    return boolField(mTbl["view"], "showSpectrogram", true);
}

void Config::setViewShowSpectrogram(bool b)
{
    mTbl["view"].as_table()->insert_or_assign("showSpectrogram", static_cast<bool>(b));
    emit viewShowSpectrogramChanged(b);
}

bool Config::getViewShowPitch()
{
    return boolField(mTbl["view"], "showPitch", true);
}

void Config::setViewShowPitch(bool b)
{
    mTbl["view"].as_table()->insert_or_assign("showPitch", static_cast<bool>(b));
    emit viewShowPitchChanged(b);
}

int Config::getAnalysisMaxFrequency()
{
    return integerField(mTbl["analysis"], "maxFrequency", 5200);
}

int Config::getAnalysisLpOffset()
{
    return integerField(mTbl["analysis"], "lpOffset", +1);
}

int Config::getAnalysisPitchSampleRate()
{
    return integerField(mTbl["analysis"], "pitchSampleRate", 32000);
}

void Config::setAnalysisGranularity(double ms) {
    mTbl["analysis"].as_table()->insert_or_assign("granularity", static_cast<double>(ms));
}

double Config::getAnalysisGranularity() {
    return doubleField(mTbl["analysis"], "granularity", 10.0);
}

void Config::setAnalysisSpectrogramWindow(double ms) {
    mTbl["analysis"].as_table()->insert_or_assign("spectrogramWindow", static_cast<double>(ms));
}

double Config::getAnalysisSpectrogramWindow() {
    return doubleField(mTbl["analysis"], "spectrogramWindow", 50.0);
}

void Config::setAnalysisPitchWindow(double ms) {
    mTbl["analysis"].as_table()->insert_or_assign("pitchWindow", static_cast<double>(ms));
}

double Config::getAnalysisPitchWindow() {
    return doubleField(mTbl["analysis"], "pitchWindow", 40.0);
}

void Config::setAnalysisFormantWindow(double ms) {
    mTbl["analysis"].as_table()->insert_or_assign("formantWindow", static_cast<double>(ms));
}

double Config::getAnalysisFormantWindow() {
    return doubleField(mTbl["analysis"], "formantWindow", 20.0);
}

void Config::setAnalysisOscilloscopeWindow(double ms) {
    mTbl["analysis"].as_table()->insert_or_assign("oscilloscopeWindow", static_cast<double>(ms));
}

double Config::getAnalysisOscilloscopeWindow() {
    return doubleField(mTbl["analysis"], "oscilloscopeWindow", 80.0);
}

void Config::setAnalysisPitchSpacing(double ms) {
    mTbl["analysis"].as_table()->insert_or_assign("pitchSpacing", static_cast<double>(ms));
}

double Config::getAnalysisPitchSpacing() {
    return doubleField(mTbl["analysis"], "pitchSpacing", 20.0);
}

void Config::setAnalysisFormantSpacing(double ms) {
    mTbl["analysis"].as_table()->insert_or_assign("formantSpacing", static_cast<double>(ms));
}

double Config::getAnalysisFormantSpacing() {
    return doubleField(mTbl["analysis"], "formantSpacing", 40.0);
}

void Config::setAnalysisOscilloscopeSpacing(double ms) {
    mTbl["analysis"].as_table()->insert_or_assign("oscilloscopeSpacing", static_cast<double>(ms));
}

double Config::getAnalysisOscilloscopeSpacing() {
    return doubleField(mTbl["analysis"], "oscilloscopeSpacing", 20.0);
}

bool Config::isPaused()
{
    return mPaused;
}

void Config::setPaused(bool p)
{
    mPaused = p;
    emit pausedChanged(p);
}
