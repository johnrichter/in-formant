#include "contextmanager.h"
#include "config.h"
#include "cfgpath.h"
#include "../gui/theme.h"
#include <iostream>

using namespace Main;

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

toml::table Main::getConfigTable()
{
    auto path = getConfigPath();

#if defined(_WIN32) && defined(UNICODE)
    std::wcout << "Reading configuration from: " << path << std::endl;
#else
    std::cout << "Reading configuration from: " << path << std::endl;
#endif

    fs::create_directories(path.parent_path());

    if (fs::exists(path) && fs::is_regular_file(path)) {
        std::ifstream ifs(path, std::ios_base::binary);
#if defined(_WIN32) && defined(UNICODE)
        const auto pathString = path.wstring();
#else
        const auto pathString = path.string();
#endif
        return toml::parse(ifs, pathString);
    }

    std::cout << "Config not found, using defaults..." << std::endl;
    return toml::table();
}

Config::Config()
    : mTbl(getConfigTable()),
      mPaused(false)
{
    initSubTable(mTbl, "audio");
    initSubTable(mTbl, "solvers");
    initSubTable(mTbl, "view");
    initSubTable(mTbl, "ui");
    initSubTable(mTbl, "analysis");
    initSubTable(mTbl, "options");
    initSubTable(mTbl, "meta");

    // The configuration file stores each default at the first exit, so a new default has no effect
    // on an existing file. Version 2 applies the new defaults for the time step and the view duration
    // one time.
    constexpr int64_t currentVersion = 2;
    const int64_t fileVersion = integerField(mTbl["meta"], "version", 0);
    if (fileVersion < 2) {
        mTbl["analysis"].as_table()->insert_or_assign("granularity", 10.0);
        // Closer frames give smoother lines in the tracks.
        mTbl["analysis"].as_table()->insert_or_assign("pitchSpacing", 20.0);
        mTbl["analysis"].as_table()->insert_or_assign("formantSpacing", 40.0);
        mTbl["view"].as_table()->insert_or_assign("timeSpan", 15.0);
    }
    mTbl["meta"].as_table()->insert_or_assign("version", currentVersion);

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
    m["showStats"] = false;
    m["showMiniMap"] = true;

    // Axes.
    m["showAxisLabels"] = true;
    m["showAxisTitles"] = true;
    m["freqTickMode"] = 0;       // 0 automatic, 1 custom step, 2 musical notes, 3 octaves
    m["freqTickStep"] = 100.0;   // Hz, for the custom step
    m["timeTickMode"] = 0;       // 0 automatic, 1 custom step
    m["timeTickStep"] = 1.0;     // seconds, for the custom step

    // Tracks.
    m["trackStyle"] = 2;         // 0 dots, 1 lines, 2 lines and dots
    m["lineSmoothness"] = 35;    // percent
    m["lineWidth"] = 2.5;        // pixels at 100 percent zoom
    m["dotRadius"] = 3.0;        // pixels at 100 percent zoom

    // Track statistics. Each bit of the mask shows one value.
    m["statsMask"] = 0x1 | 0x2 | 0x8 | 0x40 | 0x80;   // current, min, mean, p95, max
    m["statsWindow"] = 0.0;      // seconds, 0 means the visible window

    // Colour theme.
    m["theme"] = QString("Magma");
    m["customColors"] = QString("#000004,#3b0f70,#b73779,#fc8961,#fcfdbf");
    m["customPitch"] = QString("#00e5ff");
    m["customFormants"] = QString("#00ff00,#91ee91,#ff0000,#ff99ff,#ffcc00");
    return m;
}

QVariantMap Config::options() const
{
    return mOptions;
}

QVariantMap Config::themeColors() const
{
    const Gui::Theme theme = Gui::resolveTheme(optionString("theme"), optionString("customColors"),
                                               optionString("customPitch"), optionString("customFormants"));
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
                                               optionString("customFormants"));
        QVariantList list;
        for (int i = 0; i < count; ++i) {
            list << QColor::fromRgb(t.colormap[count > 1 ? (i * 255) / (count - 1) : 0]);
        }
        return list;
    }
    return Gui::themePreview(name, count);
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

bool Config::optionBool(const QString &name) const { return mOptions.value(name).toBool(); }
int Config::optionInt(const QString &name) const { return mOptions.value(name).toInt(); }
double Config::optionDouble(const QString &name) const { return mOptions.value(name).toDouble(); }
QString Config::optionString(const QString &name) const { return mOptions.value(name).toString(); }

Config::~Config()
{
    std::ofstream stream(getConfigPath());
    stream << mTbl;
}

Module::Audio::Backend Config::getAudioBackend()
{
    return enumField(toml::node_view(mTbl), "audioBackend", Main::getDefaultAudioBackend());
}

void Config::setAudioBackend(Module::Audio::Backend b)
{
    mTbl["audioBackend"].ref<int64_t>() = enumInt(b);
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

void Config::setAudioCaptureDevices(const QStringList &names)
{
    mAudioCaptureDevices = names;
    emit audioCaptureDevicesChanged();
}

PitchAlgorithm Config::getPitchAlgorithm()
{
    return enumField(mTbl["solvers"], "pitch", PitchAlgorithm::RAPT);
}

void Config::setPitchAlgorithm(PitchAlgorithm alg)
{
    mTbl["solvers"]["pitch"].ref<int64_t>() = enumInt(alg);
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
    mTbl["solvers"]["linpred"].ref<int64_t>() = enumInt(alg);
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
    mTbl["solvers"]["formant"].ref<int64_t>() = enumInt(alg);
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
    mTbl["solvers"]["invglot"].ref<int64_t>() = enumInt(alg);
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
    return doubleField(mTbl["view"], "zoomScale", 1.0);
}

void Config::setViewZoom(double scale)
{
    mTbl["view"]["zoomScale"].ref<double>() = scale;
    emit viewZoomChanged(scale);
}

int Config::getViewMinFrequency()
{
    return integerField(mTbl["view"], "minFrequency", 1);
}

void Config::setViewMinFrequency(int f)
{
    mTbl["view"]["minFrequency"].ref<int64_t>() = f;
    emit viewMinFrequencyChanged(f);
}

int Config::getViewMaxFrequency()
{
    return integerField(mTbl["view"], "maxFrequency", 6000);
}

void Config::setViewMaxFrequency(int f)
{
    mTbl["view"]["maxFrequency"].ref<int64_t>() = f;
    emit viewMaxFrequencyChanged(f);
}

int Config::getViewFFTSize()
{
    return integerField(mTbl["view"], "fftSize", 2048);
}

void Config::setViewFFTSize(int nfft)
{
    mTbl["view"]["fftSize"].ref<int64_t>() = nfft;
    emit viewFFTSizeChanged(nfft);
}

int Config::getViewMaxGain()
{
    return integerField(mTbl["view"], "maxGain", 0);
}

void Config::setViewMaxGain(int g)
{
    mTbl["view"]["maxGain"].ref<int64_t>() = g;
    emit viewMaxGainChanged(g);
}

int Config::getViewSpectrogramQuality()
{
    return integerField(mTbl["view"], "spectrogramQuality", 1);
}

void Config::setViewSpectrogramQuality(int quality)
{
    mTbl["view"]["spectrogramQuality"].ref<int64_t>() = quality;
    emit viewSpectrogramQualityChanged(quality);
}

int Config::getViewSpectrogramBlur()
{
    return integerField(mTbl["view"], "spectrogramBlur", 33);
}

void Config::setViewSpectrogramBlur(int percent)
{
    mTbl["view"]["spectrogramBlur"].ref<int64_t>() = percent;
    emit viewSpectrogramBlurChanged(percent);
}

double Config::getViewTimeSpan()
{
    return doubleField(mTbl["view"], "timeSpan", 15.0);
}

void Config::setViewTimeSpan(double dur)
{
    mTbl["view"]["timeSpan"].ref<double>() = dur;
    emit viewTimeSpanChanged(dur);
}

FrequencyScale Config::getViewFrequencyScale()
{
    return enumField(mTbl["view"], "frequencyScale", FrequencyScale::ERB);
}

void Config::setViewFrequencyScale(FrequencyScale scale)
{
    mTbl["view"]["frequencyScale"].ref<int64_t>() = enumInt(scale);
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
    return integerField(mTbl["view"], "formantCount", 4);
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
    mTbl["view"]["showSpectrogram"].ref<bool>() = b;
    emit viewShowSpectrogramChanged(b);
}

bool Config::getViewShowPitch()
{
    return boolField(mTbl["view"], "showPitch", true);
}

void Config::setViewShowPitch(bool b)
{
    mTbl["view"]["showPitch"].ref<bool>() = b;
    emit viewShowPitchChanged(b);
}

bool Config::getViewShowFormants()
{
    return boolField(mTbl["view"], "showFormants", true);
}

void Config::setViewShowFormants(bool b)
{
    mTbl["view"]["showFormants"].ref<bool>() = b;
    emit viewShowFormantsChanged(b);
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
    mTbl["analysis"]["granularity"].ref<double>() = ms;
}

double Config::getAnalysisGranularity() {
    return doubleField(mTbl["analysis"], "granularity", 10.0);
}

void Config::setAnalysisSpectrogramWindow(double ms) {
    mTbl["analysis"]["spectrogramWindow"].ref<double>() = ms;
}

double Config::getAnalysisSpectrogramWindow() {
    return doubleField(mTbl["analysis"], "spectrogramWindow", 50.0);
}

void Config::setAnalysisPitchWindow(double ms) {
    mTbl["analysis"]["pitchWindow"].ref<double>() = ms;
}

double Config::getAnalysisPitchWindow() {
    return doubleField(mTbl["analysis"], "pitchWindow", 40.0);
}

void Config::setAnalysisFormantWindow(double ms) {
    mTbl["analysis"]["formantWindow"].ref<double>() = ms;
}

double Config::getAnalysisFormantWindow() {
    return doubleField(mTbl["analysis"], "formantWindow", 20.0);
}

void Config::setAnalysisOscilloscopeWindow(double ms) {
    mTbl["analysis"]["oscilloscopeWindow"].ref<double>() = ms;
}

double Config::getAnalysisOscilloscopeWindow() {
    return doubleField(mTbl["analysis"], "oscilloscopeWindow", 80.0);
}

void Config::setAnalysisPitchSpacing(double ms) {
    mTbl["analysis"]["pitchSpacing"].ref<double>() = ms;
}

double Config::getAnalysisPitchSpacing() {
    return doubleField(mTbl["analysis"], "pitchSpacing", 20.0);
}

void Config::setAnalysisFormantSpacing(double ms) {
    mTbl["analysis"]["formantSpacing"].ref<double>() = ms;
}

double Config::getAnalysisFormantSpacing() {
    return doubleField(mTbl["analysis"], "formantSpacing", 40.0);
}

void Config::setAnalysisOscilloscopeSpacing(double ms) {
    mTbl["analysis"]["oscilloscopeSpacing"].ref<double>() = ms;
}

double Config::getAnalysisOscilloscopeSpacing() {
    return doubleField(mTbl["analysis"], "oscilloscopeSpacing", 160.0);
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
