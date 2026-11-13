#include "rendercontext.h"
#include "../gui/bands.h"
#include "config.h"
#include "../context/timings.h"
#include <iostream>

using namespace Main;

RenderContext::RenderContext(Config *config, DataStore *dataStore)
    : mConfig(config),
      mDataStore(dataStore),
      mSelectedView(nullptr)
{
}

void RenderContext::render(QPainterWrapper *painter)
{
    timer_guard timer(timings::render);

    painter->setZoom(mConfig->getViewZoom());
    painter->setMinFrequency(mConfig->getViewMinFrequency());
    painter->setMaxFrequency(mConfig->getViewMaxFrequency());
    painter->setFrequencyScale(mConfig->getViewFrequencyScale());
    painter->setMaxGain(mConfig->getViewMaxGain());
    painter->setSpectrogramFilter(mConfig->getViewSpectrogramQuality(), mConfig->getViewSpectrogramBlur() / 100.0);

    // The options of the display.
    Gui::RenderOptions options;
    options.showDebug = mConfig->optionBool("showDebug");
    options.showAxisLabels = mConfig->optionBool("showAxisLabels");
    options.showAxisTitles = mConfig->optionBool("showAxisTitles");
    options.showLegend = mConfig->optionBool("showLegend");
    options.showStats = mConfig->optionBool("showStats");
    options.freqTickMode = mConfig->optionInt("freqTickMode");
    options.freqTickStep = mConfig->optionDouble("freqTickStep");
    options.timeTickMode = mConfig->optionInt("timeTickMode");
    options.timeTickStep = mConfig->optionDouble("timeTickStep");
    options.hideDebug = mConfig->isRecording();
    // The recorder grabs the whole window, so the zones on the screen are also in the video.
    options.timeFlipped = mConfig->optionBool("flipTime");
    options.gridStrength = mConfig->optionDouble("gridStrength");
    options.freqTickDensity = mConfig->optionInt("freqTickDensity");
    options.gridMajor = mConfig->optionBool("gridMajor");
    options.gridMinor = mConfig->optionBool("gridMinor");
    options.showBands = mConfig->optionBool("showBands") && (mConfig->optionBool("bandsInExport") || !mConfig->isRecording());
    const Gui::BandEdges bandEdges = Gui::parseBandEdges(mConfig->optionString("bandEdges").toStdString());
    for (int i = 0; i < 4; ++i) options.bandEdges[i] = bandEdges[i];
    options.readoutOrder = mConfig->optionString("readoutOrder").toStdString();
    options.readoutLeft = mConfig->optionString("readoutSide") == "left";
    const char *boxKeys[5] = { "showBoxPitch", "showBoxF1", "showBoxF2", "showBoxF3", "showBoxF4" };
    const char *styleKeys[5] = { "trackStylePitch", "trackStyleF1", "trackStyleF2", "trackStyleF3", "trackStyleF4" };
    for (int i = 0; i < 5; ++i) {
        options.trackBoxShown[i] = mConfig->optionBool(boxKeys[i]);
        options.trackStyle[i] = mConfig->optionInt(styleKeys[i]);
    }
    options.lineSmoothness = mConfig->optionInt("lineSmoothness") / 100.0;
    options.lineWidth = mConfig->optionDouble("lineWidth");
    options.dotRadius = mConfig->optionDouble("dotRadius");
    // The pitch-only graph has one track, so the track is larger.
    if (mConfig->optionString("graphMode") == "pitch") {
        options.lineWidth *= 1.6;
        options.dotRadius *= 1.4;
    }
    options.statsMask = mConfig->optionInt("statsMask");
    options.rangeSemitones = mConfig->optionBool("rangeSemitones");

    const QString themeKey = mConfig->optionString("theme") + "|" + mConfig->optionString("customColors")
        + "|" + mConfig->optionString("customPitch") + "|" + mConfig->optionString("customFormants") + "|" + mConfig->optionString("colorSaturation") + "|" + mConfig->optionString("minVolumeBlack");
    if (themeKey != mThemeKey) {
        mThemeKey = themeKey;
        mTheme = Gui::resolveTheme(mConfig->optionString("theme"), mConfig->optionString("customColors"),
                                   mConfig->optionString("customPitch"), mConfig->optionString("customFormants"),
                                   mConfig->optionDouble("colorSaturation"), mConfig->optionBool("minVolumeBlack"));
    }
    options.theme = mTheme;

    painter->setOptions(options);

    if (mSelectedView != nullptr) {
        mSelectedView->render(painter, mConfig, mDataStore);
    }
}

void RenderContext::setView(RenderView *view)
{
    mSelectedView = view;
}
