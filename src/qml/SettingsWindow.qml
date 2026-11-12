import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: settingsWindow
    title: appName + " - Settings"
    visible: false

    width: 560
    height: 680
    minimumWidth: 460
    minimumHeight: 420

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    // One switch for a true or false option.
    component OptionSwitch: Switch {
        property string key
        checked: config.options[key]
        onToggled: config.setOption(key, checked)
    }

    // One slider for a number option. The label shows the value and the unit.
    component OptionSlider: ColumnLayout {
        property string key
        property string title
        property string unit: ""
        property real from: 0
        property real to: 100
        property real stepSize: 1
        property int decimals: 0
        Layout.fillWidth: true
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Label { text: title; Layout.fillWidth: true }
            Label { text: Number(config.options[key]).toFixed(decimals) + unit; color: "lightgrey" }
        }
        Slider {
            Layout.fillWidth: true
            from: parent.from
            to: parent.to
            stepSize: parent.stepSize
            value: config.options[key]
            onMoved: config.setOption(key, value)
        }
    }

    // One check box for one bit of the mask of the statistics.
    component StatBox: CheckBox {
        property int bit
        checked: (config.options.statsMask & bit) !== 0
        onToggled: config.setOption("statsMask", checked ? (config.options.statsMask | bit) : (config.options.statsMask & ~bit))
    }

    header: TabBar {
        id: tabs
        TabButton { text: "Axes" }
        TabButton { text: "Tracks" }
        TabButton { text: "Readouts" }
        TabButton { text: "Theme" }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

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

                Label { text: "How to draw the pitch track and the formant tracks"; font.bold: true; Layout.topMargin: 14 }
                ComboBox {
                    Layout.fillWidth: true
                    model: [ "Dots", "Lines", "Lines and dots" ]
                    currentIndex: config.options.trackStyle
                    onActivated: config.setOption("trackStyle", currentIndex)
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

                Label { text: "On the graph"; font.bold: true; Layout.topMargin: 14 }
                OptionSwitch { key: "showLegend"; text: "Legend" }
                OptionSwitch { key: "showDebug"; text: "Frame times (render, update, frames per second)" }
                OptionSwitch { key: "showStats"; text: "Statistics of the tracks" }

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
                    text: "The theme sets the colours of the spectrogram, the pitch track, the formant tracks, and the plots of the oscilloscope and the synthesizer. Every theme works on a dark background. The pride themes use softer versions of the flag colours."
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
