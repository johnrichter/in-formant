import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: settingsWindow
    title: appName + " - Settings"
    visible: false

    width: 640
    height: 680
    minimumWidth: 460
    minimumHeight: 420

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    function mel(f) {
        return 2595 * Math.log10(1 + f / 700);
    }

    function hz(m) {
        return 700 * (Math.pow(10, m / 2595) - 1);
    }

    // One slider for a number of the configuration. The label shows the title and the value.
    component ConfigSlider: ColumnLayout {
        id: cs
        property string title
        property string valueText
        property real from: 0
        property real to: 100
        property real stepSize: 0
        property real value: 0
        signal moved(real newValue)
        Layout.fillWidth: true
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Label { text: cs.title; Layout.fillWidth: true }
            Label { text: cs.valueText; color: "lightgrey" }
        }
        Slider {
            Layout.fillWidth: true
            from: cs.from
            to: cs.to
            stepSize: cs.stepSize
            value: cs.value
            onMoved: cs.moved(value)
        }
    }

    // One list of choices for a number of the configuration.
    component ConfigCombo: ColumnLayout {
        id: cc
        property string title
        property alias model: box.model
        property int index: 0
        signal picked(int newIndex)
        Layout.fillWidth: true
        spacing: 2

        Label { text: cc.title }
        ComboBox {
            id: box
            Layout.fillWidth: true
            currentIndex: cc.index
            onActivated: cc.picked(currentIndex)
        }
    }

    // One check box for one bit of the style of a track: 1 for the dots, 2 for the line.
    component StyleBox: CheckBox {
        property string key
        property int bit
        checked: (config.options[key] & bit) !== 0
        onToggled: config.setOption(key, checked ? (config.options[key] | bit) : (config.options[key] & ~bit))
    }

    // The order of the readout boxes. A key that the stored order omits goes to the end.
    readonly property var readoutNames: ({ legend: "Legend", pitch: "Pitch", f1: "F1", f2: "F2", f3: "F3", f4: "F4" })
    readonly property var readoutSwitch: ({ legend: "showLegend", pitch: "showBoxPitch", f1: "showBoxF1",
                                            f2: "showBoxF2", f3: "showBoxF3", f4: "showBoxF4" })

    function readoutOrder() {
        const all = ["legend", "pitch", "f1", "f2", "f3", "f4"]
        const have = String(config.options.readoutOrder).split(",")
            .map(k => k.trim().toLowerCase())
            .filter((k, i, a) => all.indexOf(k) >= 0 && a.indexOf(k) === i)
        for (const k of all) {
            if (have.indexOf(k) < 0) have.push(k)
        }
        return have
    }

    function moveReadout(index, step) {
        const order = readoutOrder()
        const target = index + step
        if (target < 0 || target >= order.length) return
        const moved = order[index]
        order[index] = order[target]
        order[target] = moved
        config.setOption("readoutOrder", order.join(","))
    }

    // One check box for one bit of the mask of the statistics.
    component StatBox: CheckBox {
        property int bit
        checked: (config.options.statsMask & bit) !== 0
        onToggled: config.setOption("statsMask", checked ? (config.options.statsMask | bit) : (config.options.statsMask & ~bit))
    }

    header: TabBar {
        id: tabs
        objectName: "settingsTabs"
        TabButton { text: "General" }
        TabButton { text: "Display" }
        TabButton { text: "Input" }
        TabButton { text: "Axes" }
        TabButton { text: "Tracks" }
        TabButton { text: "Readouts" }
        TabButton { text: "Theme" }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        // General
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 40
                x: 20
                spacing: 8

                Label { text: "Scratch recording"; font.bold: true; Layout.topMargin: 14 }
                OptionSwitch { key: "keepRecordings"; text: "Keep all recordings when the app closes" }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "lightgrey"
                    text: "The app stores the live sound in a scratch file. The app deletes this file when it closes. When you switch this on, the app saves the sound as a WAV file in its data folder instead. Files that you save yourself stay in all cases."
                }

                MenuSeparator { Layout.fillWidth: true }

                Label { text: "Keys"; font.bold: true }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "lightgrey"
                    text: "Space starts and stops the capture. The key combination for the preferences (Command and comma on a Mac) opens this window."
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "lightgrey"
                    text: "The Record button below the graph asks for the choices of the video at each start. The time of the view is a field at the right end of the bar below the graph."
                }
            }
        }

        // Display
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 40
                x: 20
                spacing: 8

                Label { text: "Layers"; font.bold: true; Layout.topMargin: 14 }
                Switch {
                    text: "Spectrogram"
                    checked: config.viewShowSpectrogram
                    onToggled: config.viewShowSpectrogram = checked
                }
                Switch {
                    text: "Pitch track"
                    checked: config.viewShowPitch
                    onToggled: config.viewShowPitch = checked
                }
                Switch {
                    text: "Formant tracks"
                    checked: config.viewShowFormants
                    onToggled: config.viewShowFormants = checked
                }

                MenuSeparator { Layout.fillWidth: true }

                Label { text: "Frequency range of the view"; font.bold: true }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: config.viewMinFrequency + " Hz"; color: "lightgrey" }
                    Item { Layout.fillWidth: true }
                    Label { text: config.viewMaxFrequency + " Hz"; color: "lightgrey" }
                }
                RangeSlider {
                    Layout.fillWidth: true
                    from: settingsWindow.mel(1)
                    to: settingsWindow.mel(16000)
                    first.value: settingsWindow.mel(config.viewMinFrequency)
                    first.onMoved: config.viewMinFrequency = Math.max(1, settingsWindow.hz(first.value))
                    second.value: settingsWindow.mel(config.viewMaxFrequency)
                    second.onMoved: config.viewMaxFrequency = Math.max(100, settingsWindow.hz(second.value))
                }
                ConfigCombo {
                    title: "Frequency scale"
                    model: [ "Linear", "Logarithmic", "Mel", "ERB" ]
                    index: config.viewFrequencyScale
                    onPicked: (i) => config.viewFrequencyScale = i
                }

                MenuSeparator { Layout.fillWidth: true }

                Label { text: "Spectrogram"; font.bold: true }
                ConfigSlider {
                    title: "FFT size"
                    valueText: String(config.viewFFTSize)
                    from: Math.log2(512); to: Math.log2(4096); stepSize: 1
                    value: Math.log2(config.viewFFTSize)
                    onMoved: (v) => config.viewFFTSize = Math.pow(2, Math.round(v))
                }
                ConfigSlider {
                    title: "Gain normalization"
                    valueText: config.viewMaxGain + " dB"
                    from: -100; to: 40
                    value: config.viewMaxGain
                    onMoved: (v) => config.viewMaxGain = v
                }
                ConfigCombo {
                    title: "Quality"
                    model: [ "Fast", "Smooth" ]
                    index: config.viewSpectrogramQuality
                    onPicked: (i) => config.viewSpectrogramQuality = i
                }
                ConfigSlider {
                    enabled: config.viewSpectrogramQuality > 0
                    title: "Blur"
                    valueText: config.viewSpectrogramBlur + " %"
                    from: 0; to: 100; stepSize: 1
                    value: config.viewSpectrogramBlur
                    onMoved: (v) => config.viewSpectrogramBlur = v
                }
            }
        }

        // Input
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 40
                x: 20
                spacing: 8

                Label { text: "Input device"; font.bold: true; Layout.topMargin: 14 }
                ComboBox {
                    Layout.fillWidth: true
                    model: ["System default" + (config.audioDefaultCaptureDevice !== "" ? " (" + config.audioDefaultCaptureDevice + ")" : "")]
                            .concat(config.audioCaptureDevices)
                    currentIndex: Math.max(0, model.indexOf(config.audioCaptureDevice))
                    onActivated: config.audioCaptureDevice = (currentIndex === 0 ? "" : model[currentIndex])
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "lightgrey"
                    text: "The system default follows the choice in the sound settings of the system. The app switches to a new default device by itself."
                }

                MenuSeparator { Layout.fillWidth: true }

                Label { text: "Algorithms"; font.bold: true }
                ConfigCombo {
                    title: "Pitch"
                    model: [ "YIN", "McLeod", "RAPT" ]
                    index: config.pitchAlgorithm
                    onPicked: (i) => config.pitchAlgorithm = i
                }
                ConfigCombo {
                    title: "Formants"
                    model: HAS_TORCH ? [ "Simple LPC", "Filtered LPC", "DeepFormants" ] : [ "Simple LPC", "Filtered LPC" ]
                    index: config.formantAlgorithm
                    onPicked: (i) => config.formantAlgorithm = i
                }
                ConfigCombo {
                    title: "Linear prediction (LPC)"
                    model: [ "Autocorrelation", "Covariance", "Burg" ]
                    index: config.linpredAlgorithm
                    onPicked: (i) => config.linpredAlgorithm = i
                }
                ConfigCombo {
                    title: "Glottal inverse filtering"
                    model: [ "IAIF", "GFM-IAIF" ]
                    index: config.invglotAlgorithm
                    onPicked: (i) => config.invglotAlgorithm = i
                }
            }
        }

        // Axes
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 40
                x: 20
                spacing: 8

                Label { text: "Labels"; font.bold: true; Layout.topMargin: 14 }
                OptionSwitch { key: "showAxisLabels"; text: "Show the numbers on the axes" }
                OptionSwitch { key: "showAxisTitles"; text: "Show the names of the axes" }

                MenuSeparator { Layout.fillWidth: true }

                Label { text: "Frequency axis (vertical)"; font.bold: true }
                ComboBox {
                    Layout.fillWidth: true
                    model: [ "Automatic: round values that fit the height",
                             "Custom step",
                             "Musical notes",
                             "Octaves (C notes only)" ]
                    currentIndex: config.options.freqTickMode
                    onActivated: config.setOption("freqTickMode", currentIndex)
                }
                RowLayout {
                    visible: config.options.freqTickMode === 1
                    Label { text: "Step (Hz)" }
                    TextField {
                        Layout.fillWidth: true
                        text: config.options.freqTickStep
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        validator: DoubleValidator { bottom: 0.001; top: 100000 }
                        onEditingFinished: config.setOption("freqTickStep", parseFloat(text))
                    }
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "lightgrey"
                    text: "Automatic mode labels the roundest values first (1000, 500, 200, 100) and fills the free space with finer values. The labels never overlap, at any window height."
                }

                MenuSeparator { Layout.fillWidth: true }

                Label { text: "Time axis (horizontal)"; font.bold: true }
                ComboBox {
                    Layout.fillWidth: true
                    model: [ "Automatic: round values that fit the width", "Custom step" ]
                    currentIndex: config.options.timeTickMode
                    onActivated: config.setOption("timeTickMode", currentIndex)
                }
                RowLayout {
                    visible: config.options.timeTickMode === 1
                    Label { text: "Step (seconds)" }
                    TextField {
                        Layout.fillWidth: true
                        text: config.options.timeTickStep
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        validator: DoubleValidator { bottom: 0.001; top: 86400 }
                        onEditingFinished: config.setOption("timeTickStep", parseFloat(text))
                    }
                }
            }
        }

        // Tracks
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 40
                x: 20
                spacing: 8

                Label { text: "How to draw each track"; font.bold: true; Layout.topMargin: 14 }
                GridLayout {
                    columns: 3
                    columnSpacing: 24
                    Layout.fillWidth: true

                    Label { text: "" }
                    Label { text: "Dots"; color: "lightgrey" }
                    Label { text: "Line"; color: "lightgrey" }

                    Label { text: "Pitch" }
                    StyleBox { key: "trackStylePitch"; bit: 1 }
                    StyleBox { key: "trackStylePitch"; bit: 2 }

                    Label { text: "F1" }
                    StyleBox { key: "trackStyleF1"; bit: 1 }
                    StyleBox { key: "trackStyleF1"; bit: 2 }

                    Label { text: "F2" }
                    StyleBox { key: "trackStyleF2"; bit: 1 }
                    StyleBox { key: "trackStyleF2"; bit: 2 }

                    Label { text: "F3" }
                    StyleBox { key: "trackStyleF3"; bit: 1 }
                    StyleBox { key: "trackStyleF3"; bit: 2 }

                    Label { text: "F4" }
                    StyleBox { key: "trackStyleF4"; bit: 1 }
                    StyleBox { key: "trackStyleF4"; bit: 2 }
                }

                OptionSlider {
                    key: "lineSmoothness"; title: "Line smoothness"; unit: " %"; from: 0; to: 100
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "lightgrey"
                    text: "0 % joins the measured points with straight lines. A higher value averages the neighbouring points and joins them with a smooth curve. The dots always show the measured values."
                }
                OptionSlider {
                    key: "lineWidth"; title: "Line width"; unit: " px"; from: 1; to: 8; stepSize: 0.5; decimals: 1
                }
                OptionSlider {
                    key: "dotRadius"; title: "Dot size"; unit: " px"; from: 1; to: 8; stepSize: 0.5; decimals: 1
                }
            }
        }

        // Readouts
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 40
                x: 20
                spacing: 4

                Label { text: "Boxes"; font.bold: true; Layout.topMargin: 14 }
                ConfigCombo {
                    title: "Place of the boxes"
                    model: [ "Below the graph", "At the left of the graph" ]
                    index: config.options.readoutSide === "left" ? 1 : 0
                    onPicked: (i) => config.setOption("readoutSide", i === 1 ? "left" : "bottom")
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "lightgrey"
                    text: "The boxes follow the order of this list. A box for a track shows the statistics of that track. A track that the graph does not show has no box."
                }
                Repeater {
                    model: settingsWindow.readoutOrder()
                    RowLayout {
                        Layout.fillWidth: true
                        required property string modelData
                        required property int index
                        ToolButton {
                            text: "\u25B2"
                            enabled: index > 0
                            onClicked: settingsWindow.moveReadout(index, -1)
                        }
                        ToolButton {
                            text: "\u25BC"
                            enabled: index < 5
                            onClicked: settingsWindow.moveReadout(index, 1)
                        }
                        Label { text: settingsWindow.readoutNames[modelData]; Layout.fillWidth: true }
                        OptionSwitch { key: settingsWindow.readoutSwitch[modelData]; text: "" }
                    }
                }

                MenuSeparator { Layout.fillWidth: true }

                Label { text: "On the graph"; font.bold: true }
                OptionSwitch { key: "showDebug"; text: "Frame times, in a box at the top of the graph" }
                OptionSwitch { key: "showStats"; text: "Statistics of the tracks" }
                OptionSwitch { key: "showMiniMap"; text: "Map of the session, below the graph" }
                OptionSlider {
                    enabled: config.options.showMiniMap
                    key: "mapWindow"; title: "Time in the map"; unit: " s"; from: 30; to: 900; stepSize: 30
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "lightgrey"
                    text: "A session that is shorter than this time shows whole in the map. A longer session shows the newest part, and a strip under the map scrolls it. Drag an edge of the frame in the map to change the time of the view."
                }

                MenuSeparator { Layout.fillWidth: true }

                Label { text: "Statistics to show"; font.bold: true }
                GridLayout {
                    columns: 2
                    enabled: config.options.showStats
                    opacity: enabled ? 1.0 : 0.5
                    Layout.fillWidth: true
                    StatBox { bit: 0x1; text: "Current value" }
                    StatBox { bit: 0x2; text: "Minimum" }
                    StatBox { bit: 0x4; text: "5th percentile (low, no extremes)" }
                    StatBox { bit: 0x8; text: "Mean" }
                    StatBox { bit: 0x10; text: "Median" }
                    StatBox { bit: 0x20; text: "Standard deviation" }
                    StatBox { bit: 0x40; text: "95th percentile (high, no extremes)" }
                    StatBox { bit: 0x80; text: "Maximum" }
                    StatBox { bit: 0x100; text: "Range (P95 minus P5)" }
                    StatBox { bit: 0x800; text: "Variation (SD / mean)" }
                    StatBox { bit: 0x200; text: "Voiced share" }
                    StatBox { bit: 0x1000; text: "Number of values" }
                    StatBox { bit: 0x400; text: "Pitch note and cents" }
                }

                RowLayout {
                    enabled: config.options.showStats
                    Label { text: "Window of the statistics" }
                    ComboBox {
                        Layout.fillWidth: true
                        model: [ "The visible window", "The last 5 seconds", "The last 10 seconds", "The last 30 seconds", "The last 60 seconds" ]
                        readonly property var seconds: [0, 5, 10, 30, 60]
                        currentIndex: Math.max(0, seconds.indexOf(config.options.statsWindow))
                        onActivated: config.setOption("statsWindow", seconds[currentIndex])
                    }
                }
            }
        }

        // Theme
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 40
                x: 20
                spacing: 8

                Label { text: "Colour theme"; font.bold: true; Layout.topMargin: 14 }
                ComboBox {
                    id: themeBox
                    Layout.fillWidth: true
                    model: config.themeNames()
                    currentIndex: Math.max(0, config.themeNames().indexOf(config.options.theme))
                    onActivated: config.setOption("theme", model[currentIndex])
                }

                // The preview of the colour map, from quiet to loud.
                Row {
                    Layout.fillWidth: true
                    height: 36
                    Repeater {
                        model: config.themePreview(config.options.theme, 48)
                        Rectangle {
                            width: (settingsWindow.width - 40) / 48
                            height: 36
                            color: modelData
                        }
                    }
                }

                ConfigSlider {
                    title: "Saturation"
                    valueText: Math.round(100 * config.options.colorSaturation) + "%"
                    from: 0.5; to: 1.5; stepSize: 0.05
                    value: config.options.colorSaturation
                    onMoved: (v) => config.setOption("colorSaturation", Math.round(v * 20) / 20)
                }

                // The colours of the tracks.
                RowLayout {
                    Label { text: "Pitch"; color: "lightgrey" }
                    Rectangle { width: 22; height: 22; radius: 11; color: config.themeColors.pitch; border.color: "black" }
                    Label { text: "Formants"; color: "lightgrey"; Layout.leftMargin: 12 }
                    Repeater {
                        model: 5
                        Rectangle { width: 22; height: 22; radius: 11; color: config.themeColors.formants[index]; border.color: "black" }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "lightgrey"
                    text: "The theme sets the colours of the spectrogram, the pitch track, the formant tracks, and the plots of the oscilloscope and the synthesizer. Every theme works on a dark background. The pride themes use the flag colours at full strength, from dark to light. The brightness of the spectrogram has its own slider (Gain normalization) in the Display tab."
                }

                MenuSeparator { Layout.fillWidth: true; visible: config.options.theme === "Custom" }

                ColumnLayout {
                    visible: config.options.theme === "Custom"
                    Layout.fillWidth: true
                    Label { text: "Custom theme"; font.bold: true }
                    Label { text: "Spectrogram colours, from quiet to loud (#rrggbb, separated by commas)"; color: "lightgrey" }
                    TextField {
                        Layout.fillWidth: true
                        text: config.options.customColors
                        onEditingFinished: config.setOption("customColors", text)
                    }
                    Label { text: "Pitch track colour"; color: "lightgrey" }
                    TextField {
                        Layout.fillWidth: true
                        text: config.options.customPitch
                        onEditingFinished: config.setOption("customPitch", text)
                    }
                    Label { text: "Formant track colours (F1 to F5)"; color: "lightgrey" }
                    TextField {
                        Layout.fillWidth: true
                        text: config.options.customFormants
                        onEditingFinished: config.setOption("customFormants", text)
                    }
                }
            }
        }
    }
}
