import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: settingsWindow
    title: appName + " - Settings"
    visible: false

    // The values go to the file when the window closes, so a crash or a forced quit does not lose them.
    onVisibleChanged: if (!visible) config.save()

    width: 640
    height: 680
    minimumWidth: 460
    minimumHeight: 420

    font.pixelSize: ui.type.body
    Material.theme: Material.Dark
    // One corner radius for buttons, fields, lists, and popups. It is the medium radius of the design scale.
    Material.roundedScale: Material.SmallScale
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
        property string hint: ""
        property string more: ""
        signal moved(real newValue)
        Layout.fillWidth: true
        spacing: ui.space.xs

        RowLayout {
            Layout.fillWidth: true
            Label { text: cs.title; Layout.fillWidth: true; font.weight: Font.Medium }
            Label { text: cs.valueText; color: ui.color.secondary; font.family: ui.font.mono }
            HelpButton { title: cs.title; text: cs.more }
        }
        Slider {
            Layout.fillWidth: true
            from: cs.from
            to: cs.to
            stepSize: cs.stepSize
            value: cs.value
            onMoved: cs.moved(value)
        }
        Hint { visible: cs.hint !== ""; text: cs.hint }
    }

    // One list of choices for a number of the configuration.
    component ConfigCombo: ColumnLayout {
        id: cc
        property string title
        property alias model: box.model
        property int index: 0
        property string hint: ""
        // One sentence for each choice. The sentence of the chosen item shows under the list.
        property var choiceHints: []
        property string more: ""
        signal picked(int newIndex)
        Layout.fillWidth: true
        spacing: ui.space.xs

        RowLayout {
            Layout.fillWidth: true
            Label { text: cc.title; font.weight: Font.Medium; Layout.fillWidth: true }
            HelpButton { title: cc.title; text: cc.more }
        }
        ComboBox {
            id: box
            Layout.fillWidth: true
            currentIndex: cc.index
            onActivated: cc.picked(currentIndex)
        }
        Hint { visible: cc.choiceHints.length > cc.index && cc.index >= 0; text: cc.choiceHints[cc.index] || "" }
        Hint { visible: cc.hint !== ""; text: cc.hint }
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
        // The legend is not in the list. It has its own place in the bar at the top of the graph.
        const all = ["pitch", "f1", "f2", "f3", "f4"]
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

    Dialog {
        id: resetDialog
        Material.roundedScale: Material.SmallScale
        objectName: "resetDialog"
        title: "Reset all settings?"
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel

        Component.onCompleted: standardButton(Dialog.Ok).text = "Reset"
        onAccepted: config.resetAll()

        Label {
            width: 420
            wrapMode: Text.WordWrap
            text: "Every setting returns to its default value. You cannot undo this. Your sessions, recordings, and files stay."
        }
    }

    header: AppTabBar {
        id: tabs
        objectName: "settingsTabs"
        AppTabButton { text: "General" }
        AppTabButton { text: "Display" }
        AppTabButton { text: "Input" }
        AppTabButton { text: "Axes" }
        AppTabButton { text: "Tracks" }
        AppTabButton { text: "Readouts" }
        AppTabButton { text: "Theme" }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        // General
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 2 * ui.space.xl
                x: ui.space.xl
                spacing: ui.space.md

                Heading { text: "Scratch recording"; first: true }
                OptionSwitch {
                    key: "keepRecordings"; text: "Keep all recordings when the app closes"
                    hint: "The app stores the live sound in a scratch file and deletes it at close. When this is on, the app saves the sound as a WAV file in its data folder instead. Files that you save yourself always stay."
                }


                Heading { text: "Practice" }
                Label { text: "Folder of the practice sessions" }
                TextField {
                    Layout.fillWidth: true
                    font.family: ui.font.mono
                    text: config.options.practiceFolder
                    placeholderText: practice.folder
                    onEditingFinished: config.setOption("practiceFolder", text)
                }
                Hint { text: "Leave the field empty to use Documents/InFormant/Practice. Type a full path to use another folder." }
                OptionSlider {
                    key: "practiceMinSeconds"; title: "Shortest practice that the app can save"; unit: " s"; from: 2; to: 30; stepSize: 1
                    hint: "A practice that is shorter than this time is dropped. A practice that is longer asks if you want to keep it."
                }
                OptionSwitch {
                    key: "practiceKeepAudio"; text: "Save the sound of each practice session"
                    hint: "When this is off, a session keeps only the statistics, the marks, and the report. This saves disk space."
                }
                RowLayout {
                    AppButton { text: "Open the folder"; onClicked: practice.openFolder() }
                    AppButton { text: "Export all sessions"; onClicked: practice.exportAll() }
                }
                Hint {
                    text: "Each session has its own folder with the statistics, a report, the marks, and the sound. The statistics always stay. The first write to the Documents folder can show a privacy question of macOS."
                }


                Heading { text: "Keys" }
                Hint {
                    text: "Space starts and stops the capture. The key combination for the preferences (Command and comma on a Mac) opens this window. The M key sets a mark while a practice session runs."
                }
                Hint {
                    text: "The Record button below the graph asks for the choices of the video at each start. The time of the view is a field at the right end of the bar below the graph."
                }


                Heading { text: "Reset" }
                AppButton {
                    objectName: "resetButton"
                    text: "Reset all settings to defaults"
                    onClicked: resetDialog.open()
                }
                Hint {
                    text: "This returns every setting in this window, and the view, the layers, the algorithms, and the input device, to the values of a new installation. The folder of the practice sessions stays. Your sessions, recordings, and files stay."
                }
                // Space below the last control, so the text does not touch the edge of the window.
                Item { Layout.preferredHeight: ui.space.xxl }
            }
        }

        // Display
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 2 * ui.space.xl
                x: ui.space.xl
                spacing: ui.space.md

                Heading { text: "Layers"; first: true }
                Switch {
                    text: "Spectrogram"
                    checked: config.viewShowSpectrogram
                    onToggled: config.viewShowSpectrogram = checked
                }
                Hint { text: "The color picture of the sound over time."; Layout.topMargin: -ui.space.xs; Layout.leftMargin: ui.space.xs }
                Switch {
                    text: "Pitch track"
                    checked: config.viewShowPitch
                    onToggled: config.viewShowPitch = checked
                }
                Hint { text: "The line and the dots of the pitch."; Layout.topMargin: -ui.space.xs; Layout.leftMargin: ui.space.xs }
                OptionSwitch {
                    key: "showTrackF1"; text: "Formant track F1"
                    hint: "Jaw opening. It goes up when the mouth opens."
                }
                OptionSwitch {
                    key: "showTrackF2"; text: "Formant track F2"
                    hint: "Tongue position. It goes up when the tongue moves forward."
                }
                OptionSwitch {
                    key: "showTrackF3"; text: "Formant track F3"
                    hint: "Lip shape. It goes up when the lips spread."
                }
                OptionSwitch {
                    key: "showTrackF4"; text: "Formant track F4"
                    hint: "Vocal tract length. It goes up when the tract is shorter."
                }


                Heading { text: "Pitch-only graph" }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "From"; color: ui.color.secondary }
                    TextField {
                        Layout.preferredWidth: 80
                        font.family: ui.font.mono
                        text: String(config.options.pitchAxisMin)
                        validator: DoubleValidator { bottom: 0; top: 1950; decimals: 0; notation: DoubleValidator.StandardNotation }
                        onEditingFinished: { const v = parseFloat(text); if (!isNaN(v)) config.setOption("pitchAxisMin", v) }
                    }
                    Label { text: "to"; color: ui.color.secondary }
                    TextField {
                        Layout.preferredWidth: 80
                        font.family: ui.font.mono
                        text: String(config.options.pitchAxisMax)
                        validator: DoubleValidator { bottom: 50; top: 2000; decimals: 0; notation: DoubleValidator.StandardNotation }
                        onEditingFinished: { const v = parseFloat(text); if (!isNaN(v)) config.setOption("pitchAxisMax", v) }
                    }
                    Label { text: "Hz"; color: ui.color.secondary }
                }
                Hint { text: "The vertical range of the graph in the pitch-only mode. The default is 0 to 500 Hz." }
                OptionSwitch {
                    key: "pitchAxisAuto"; text: "Fit the range to the pitch in view (logarithmic scale)"
                    hint: "When this is on, the range follows your pitch, and the two fields above do not apply."
                }


                Heading { text: "Pitch zones" }
                OptionSwitch {
                    key: "showBands"; text: "Show the pitch zones in the graph"
                    hint: "Three colored bands behind the tracks show ranges of pitch."
                }
                OptionSwitch {
                    key: "bandsInExport"; text: "Keep the zones in recordings"
                    hint: "When this is off, the zones are hidden while a recording runs."
                }
                Label { text: "Edges of the zones in Hz" }
                TextField {
                    Layout.fillWidth: true
                    font.family: ui.font.mono
                    text: config.options.bandEdges
                    onEditingFinished: config.setOption("bandEdges", text)
                }
                Hint { text: "Four values: the low edge, two inner edges, and the high edge. For example: 60, 150, 180, 300." }
                Hint {
                    text: "The zones show ranges of pitch that many listeners hear as masculine (blue), androgynous (yellow-green), or feminine (pink). The ranges are a rough stereotype. They differ between languages, voices, and listeners. A zone is not a goal. The recorder grabs the whole window, so the zones on the screen are also in a video unless you turn off the second switch."
                }


                Heading { text: "Frequency range of the view" }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: config.viewMinFrequency + " Hz"; color: ui.color.secondary; font.family: ui.font.mono }
                    Item { Layout.fillWidth: true }
                    Label { text: config.viewMaxFrequency + " Hz"; color: ui.color.secondary; font.family: ui.font.mono }
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
                Hint { text: "The lowest and the highest frequency of the view. This does not change the formant tracker." }
                ConfigCombo {
                    hint: "The scale sets how much vertical room each part of the range gets. The sound is the same on every scale."
                    choiceHints: [
                        "Linear: every hertz has the same room. Use it to count harmonics and to see breath and noise above 4 kHz.",
                        "Logarithmic: every octave has the same room, as on a piano. Use it for pitch work. A change of one semitone looks the same at every pitch.",
                        "Mel: a compromise that follows how people judge the height of a tone. The pitch and the formants F1 to F3 are all easy to see.",
                        "ERB: low and middle frequencies get the most room. Use it for resonance work. The formants F1 to F3 are well apart."
                    ]
                    more: "The scale maps a frequency in hertz to a height on the screen. On the linear scale, every hertz has the same height. On the logarithmic scale, every doubling of the frequency has the same height, as every octave has on a piano. Pitch moves in equal musical steps, so this scale fits pitch work. The Mel and ERB scales follow how people hear. They give the range of the formants (about 300 to 3500 Hz) more room than the linear scale."
                    title: "Frequency scale"
                    model: [ "Linear", "Logarithmic", "Mel", "ERB" ]
                    index: config.viewFrequencyScale
                    onPicked: (i) => config.viewFrequencyScale = i
                }


                Heading { text: "Spectrogram" }
                ConfigSlider {
                    hint: "A larger size shows finer detail in frequency and blurs fast changes. A smaller size does the opposite. Choose 512 or 1024 for fast speech and pitch work, 2048 for steady vowels and formants (the default), and 4096 for a held note."
                    more: "The FFT cuts the sound into short windows and shows the frequencies in each one. A long window holds many cycles, so it sees fine detail in frequency but coarse detail in time. A short window does the opposite. The window time is the FFT size divided by twice the highest frequency of the view. At 5500 Hz, the size 2048 gives a window of 186 ms and a line every 5.4 Hz. Use a smaller size for fast speech and a larger size for a held note."
                    title: "FFT size"
                    valueText: String(config.viewFFTSize)
                    from: Math.log2(512); to: Math.log2(4096); stepSize: 1
                    value: Math.log2(config.viewFFTSize)
                    onMoved: (v) => config.viewFFTSize = Math.pow(2, Math.round(v))
                }
                ConfigSlider {
                    hint: "The brightness of the picture. A higher value makes the picture darker. A lower value makes it brighter."
                    more: "This sets the loudness that gets the brightest color. A higher value needs more sound for a bright color, so the picture is darker. A lower value makes quiet sounds bright. Raise the value when the background noise is bright. Lower the value when your voice is faint."
                    title: "Gain normalization"
                    valueText: config.viewMaxGain + " dB"
                    from: -100; to: 40
                    value: config.viewMaxGain
                    onMoved: (v) => config.viewMaxGain = v
                }
                ConfigCombo {
                    hint: "Smooth draws a softer picture and uses more power. Fast uses less power."
                    more: "Fast draws each column of the picture as it is. Smooth blends the columns, so the picture has soft edges. Smooth uses more power from the graphics chip. On a slow computer, choose Fast."
                    title: "Quality"
                    model: [ "Fast", "Smooth" ]
                    index: config.viewSpectrogramQuality
                    onPicked: (i) => config.viewSpectrogramQuality = i
                }
                ConfigSlider {
                    hint: "More blur gives a softer picture. It works only with the quality Smooth."
                    enabled: config.viewSpectrogramQuality > 0
                    title: "Blur"
                    valueText: config.viewSpectrogramBlur + " %"
                    from: 0; to: 100; stepSize: 1
                    value: config.viewSpectrogramBlur
                    onMoved: (v) => config.viewSpectrogramBlur = v
                }
                AppButton { text: "Read the guide to these settings"; onClicked: helpWindow.showPage("choosing-settings") }
                // Space below the last control, so the text does not touch the edge of the window.
                Item { Layout.preferredHeight: ui.space.xxl }
            }
        }

        // Input
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 2 * ui.space.xl
                x: ui.space.xl
                spacing: ui.space.md

                Heading { text: "Input device"; first: true }
                ComboBox {
                    Layout.fillWidth: true
                    model: ["System default" + (config.audioDefaultCaptureDevice !== "" ? " (" + config.audioDefaultCaptureDevice + ")" : "")]
                            .concat(config.audioCaptureDevices)
                    currentIndex: Math.max(0, model.indexOf(config.audioCaptureDevice))
                    onActivated: config.audioCaptureDevice = (currentIndex === 0 ? "" : model[currentIndex])
                }
                Hint {
                    text: "The system default follows the choice in the sound settings of the system. The app switches to a new default device by itself."
                }


                Heading { text: "Algorithms" }
                ConfigCombo {
                    hint: "The method that finds the rate of your vocal folds. Try another method when the pitch track jumps by an octave."
                    choiceHints: [
                        "YIN: fast, with few errors on a clean voice. A good first choice.",
                        "McLeod: fast and stable on a clean, steady voice. It suits singing.",
                        "RAPT: looks at many frames, so the track is smoother and has fewer jumps. This is the default."
                    ]
                    more: "All three methods compare the sound with a copy of itself that moves in time. The pitch is the time shift where the two match best. A wrong match gives a jump of one octave, and the methods differ in how they avoid it. YIN corrects the match with a running mean, McLeod picks the first strong peak, and RAPT picks the best path over many frames. When the track jumps, try another method."
                    title: "Pitch"
                    model: [ "YIN", "McLeod", "RAPT" ]
                    index: config.pitchAlgorithm
                    onPicked: (i) => config.pitchAlgorithm = i
                }
                ConfigCombo {
                    hint: "The method that finds the resonances of your vocal tract (F1 to F4). Try another method when the formant tracks look wrong."
                    choiceHints: HAS_TORCH ? [
                        "Simple LPC: fast. It can show a false formant or miss a real one.",
                        "Filtered LPC: removes weak resonances and checks large gaps for a missed formant. The tracks are steadier. This is the default.",
                        "DeepFormants: a neural network finds the formants without LPC. It needs more computer power."
                    ] : [
                        "Simple LPC: fast. It can show a false formant or miss a real one.",
                        "Filtered LPC: removes weak resonances and checks large gaps for a missed formant. The tracks are steadier. This is the default."
                    ]
                    more: "A formant is a peak in the spectrum that the vocal tract makes. The LPC methods fit a smooth curve and read its peaks. The Simple method reads every peak. The Filtered method drops weak peaks and checks for a missed peak. A formant track is an estimate, and it is less exact for a high pitch. Compare a track with the spectrogram."
                    title: "Formants"
                    model: HAS_TORCH ? [ "Simple LPC", "Filtered LPC", "DeepFormants" ] : [ "Simple LPC", "Filtered LPC" ]
                    index: config.formantAlgorithm
                    onPicked: (i) => config.formantAlgorithm = i
                }
                ConfigCombo {
                    hint: "How the program fits a model of the vocal tract to the sound. Most people keep the default."
                    choiceHints: [
                        "Autocorrelation: always stable. It smooths the peaks, so a formant can look wide.",
                        "Covariance: fits a short piece of sound closely. It can become unstable, and then a formant jumps.",
                        "Burg: stable, and it keeps sharp peaks in a short window. This is the default."
                    ]
                    more: "Linear prediction fits a model of the vocal tract to a short piece of sound. The three methods are three ways to compute the model. Autocorrelation always gives a stable model. Covariance fits closely, and it can become unstable. Burg is stable, and it keeps sharp peaks in a short window. Most people do not need to change this setting."
                    title: "Linear prediction (LPC)"
                    model: [ "Autocorrelation", "Covariance", "Burg" ]
                    index: config.linpredAlgorithm
                    onPicked: (i) => config.linpredAlgorithm = i
                }
                ConfigCombo {
                    hint: "The method for the glottal flow estimate in the oscilloscope window. It does not change the main graph."
                    choiceHints: [
                        "IAIF: a classic method. It works well on a normal voice.",
                        "GFM-IAIF: a newer method with a better estimate of the spectral tilt, which relates to vocal effort. This is the default."
                    ]
                    more: "The vocal folds make a pulse of air at each cycle, and the vocal tract then shapes the sound. Inverse filtering removes the effect of the tract, to show the pulse. The oscilloscope window shows this shape. The shape tells about vocal effort and breathiness. IAIF is a classic method. GFM-IAIF models the glottal flow with a third-order filter."
                    title: "Glottal inverse filtering"
                    model: [ "IAIF", "GFM-IAIF" ]
                    index: config.invglotAlgorithm
                    onPicked: (i) => config.invglotAlgorithm = i
                }
                AppButton { text: "Read the guide to these settings"; onClicked: helpWindow.showPage("choosing-settings") }
                // Space below the last control, so the text does not touch the edge of the window.
                Item { Layout.preferredHeight: ui.space.xxl }
            }
        }

        // Axes
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 2 * ui.space.xl
                x: ui.space.xl
                spacing: ui.space.md

                Heading { text: "Labels"; first: true }
                OptionSwitch { key: "showAxisLabels"; text: "Show the numbers on the axes"; hint: "The numbers next to the ticks." }
                OptionSwitch { key: "showAxisTitles"; text: "Show the names of the axes"; hint: "The words Time and Frequency next to the axes." }


                Heading { text: "Frequency axis (vertical)" }
                ComboBox {
                    Layout.fillWidth: true
                    model: [ "Automatic: round values that fit the height",
                             "Custom step",
                             "Musical notes",
                             "Octaves (C notes only)" ]
                    currentIndex: config.options.freqTickMode
                    onActivated: config.setOption("freqTickMode", currentIndex)
                }
                Hint { text: "How the program chooses the ticks and the numbers of the frequency axis." }
                RowLayout {
                    visible: config.options.freqTickMode === 1
                    Label { text: "Step (Hz)" }
                    TextField {
                        Layout.fillWidth: true
                        font.family: ui.font.mono
                        text: config.options.freqTickStep
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        validator: DoubleValidator { bottom: 0.001; top: 100000 }
                        onEditingFinished: config.setOption("freqTickStep", parseFloat(text))
                    }
                }
                Label { text: "Number of labels (automatic mode)"; visible: config.options.freqTickMode === 0 }
                ComboBox {
                    Layout.fillWidth: true
                    visible: config.options.freqTickMode === 0
                    model: [ "Few: 100, 200, 500, 1000", "Normal: 100, 200, 300, 500, 1000", "Many: 100, 150, 200, 300, 400, 500, 1000" ]
                    currentIndex: config.options.freqTickDensity
                    onActivated: config.setOption("freqTickDensity", currentIndex)
                }
                Hint {
                    visible: config.options.freqTickMode === 0
                    text: "The automatic mode labels only round values. A label never overlaps another label, at any window height. On a linear scale the labels have equal steps (for example every 100 Hz). Choose Few for a calm axis, and Many to read a value more exactly."
                }


                Heading { text: "Grid lines" }
                OptionSwitch {
                    key: "gridMajor"; text: "Grid lines at the labelled values"
                    hint: "A line across the graph at each number of the frequency axis."
                }
                OptionSwitch {
                    key: "gridMinor"; text: "Grid lines at the values between"
                    hint: "A fainter line at each tick that has no number."
                }
                OptionSlider {
                    key: "gridStrength"; title: "Grid line strength"; unit: "%"; from: 0.1; to: 1.0; stepSize: 0.05; displayScale: 100
                    hint: "How clearly the lines show. The line color comes from the quiet color of the theme, so it fits every theme."
                    more: "The line has a light tint of the quiet color of the theme. On a light theme, it has a dark tint. A strong line is easy to see, and it can cover a faint sound. A faint line stays out of the way. The major lines have the full strength, and the minor lines have about half of it."
                }


                Heading { text: "Time axis (horizontal)" }
                ComboBox {
                    Layout.fillWidth: true
                    model: [ "Automatic: whole seconds, larger steps for longer views", "Custom step (1 second or more)" ]
                    currentIndex: config.options.timeTickMode
                    onActivated: config.setOption("timeTickMode", currentIndex)
                }
                Hint { text: "The labels are whole seconds, never fractions. A view of 15 seconds has a label each second. A view of 1 minute has a label every 5 seconds, and a view of 2 minutes every 10 seconds. A longer view has larger steps." }
                OptionSwitch {
                    key: "flipTime"; text: "Newest sound at the left"
                    hint: "When this is on, new values appear at the left edge and old values move to the right. When it is off, the graph scrolls to the left as before."
                    more: "Most graphs of speech put the newest sound at the right edge, and the old sound moves to the left. InFormant can do the opposite. The newest sound is at the left edge, and the old sound moves to the right. The time axis then counts back from the left edge. The wheel and the trackpad follow the direction. Choose the direction that is easier for you to read."
                }
                RowLayout {
                    visible: config.options.timeTickMode === 1
                    Label { text: "Step (seconds)" }
                    TextField {
                        Layout.fillWidth: true
                        font.family: ui.font.mono
                        text: config.options.timeTickStep
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        validator: DoubleValidator { bottom: 1; top: 86400 }
                        onEditingFinished: config.setOption("timeTickStep", Math.max(1, parseFloat(text)))
                    }
                }
                // Space below the last control, so the text does not touch the edge of the window.
                Item { Layout.preferredHeight: ui.space.xxl }
            }
        }

        // Tracks
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 2 * ui.space.xl
                x: ui.space.xl
                spacing: ui.space.md

                Heading { text: "How to draw each track"; first: true }
                GridLayout {
                    columns: 3
                    columnSpacing: 24
                    Layout.fillWidth: true

                    Label { text: "" }
                    Label { text: "Dots"; color: ui.color.secondary }
                    Label { text: "Line"; color: ui.color.secondary }

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

                Hint { text: "Dots show each measured value. A line joins the values." }

                OptionSlider {
                    key: "lineSmoothness"; title: "Line smoothness"; unit: " %"; from: 0; to: 100
                    hint: "0 % joins the measured points with straight lines. A higher value averages the neighbouring points and joins them with a smooth curve. The dots always show the measured values."
                }
                OptionSlider {
                    key: "lineWidth"; title: "Line width"; unit: " px"; from: 1; to: 8; stepSize: 0.5; decimals: 1
                    hint: "The thickness of the track lines, in the graph and in the legend."
                }
                OptionSlider {
                    key: "dotRadius"; title: "Dot size"; unit: " px"; from: 1; to: 8; stepSize: 0.5; decimals: 1
                    hint: "The size of the dots of the tracks."
                }
                // Space below the last control, so the text does not touch the edge of the window.
                Item { Layout.preferredHeight: ui.space.xxl }
            }
        }

        // Readouts
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 2 * ui.space.xl
                x: ui.space.xl
                spacing: ui.space.md

                Heading { text: "Boxes"; first: true }
                ConfigCombo {
                    title: "Place of the boxes"
                    model: [ "At the left of the graph", "Below the graph" ]
                    index: config.options.readoutSide === "bottom" ? 1 : 0
                    onPicked: (i) => config.setOption("readoutSide", i === 1 ? "bottom" : "left")
                    hint: "At the left, the boxes stand in a column. Below the graph, they stand in a row."
                }
                Hint {
                    text: "The boxes follow the order of this list. Below the graph, the first box is at the left. At the left of the graph, the first box is at the bottom. The column then matches the order of the tracks in the graph. A box for a track shows the statistics of that track. A track that the graph does not show has no box."
                }
                Repeater {
                    model: settingsWindow.readoutOrder()
                    RowLayout {
                        Layout.fillWidth: true
                        required property string modelData
                        required property int index
                        AppToolButton {
                            text: "\u25B2"
                            enabled: index > 0
                            onClicked: settingsWindow.moveReadout(index, -1)
                        }
                        AppToolButton {
                            text: "\u25BC"
                            enabled: index < 4
                            onClicked: settingsWindow.moveReadout(index, 1)
                        }
                        Label { text: settingsWindow.readoutNames[modelData]; Layout.fillWidth: true }
                        OptionSwitch { key: settingsWindow.readoutSwitch[modelData]; text: ""; Layout.fillWidth: false }
                    }
                }


                Heading { text: "On the graph" }
                OptionSwitch {
                    key: "showLegend"; text: "Legend, in a box in the bar above the graph"
                    hint: "The names and the colors of the tracks."
                }
                OptionSwitch {
                    key: "showDebug"; text: "Frame times, in a box in the bar above the graph"
                    hint: "Shows how long each frame takes. Use it to find out if the computer is too slow."
                }
                OptionSwitch {
                    key: "showStats"; text: "Statistics of the tracks"
                    hint: "One box for each track, with the live value and the statistics that you choose below."
                }
                OptionSwitch {
                    key: "showMiniMap"; text: "Map of the session, below the graph"
                    hint: "A picture of the whole session. A frame shows the part that the graph shows."
                }
                OptionSlider {
                    enabled: config.options.showMiniMap
                    key: "mapWindow"; title: "Time in the map"; unit: " s"; from: 30; to: 900; stepSize: 30
                    hint: "A session that is shorter than this time shows whole in the map. A longer session shows the newest part, and a strip under the map scrolls it. Drag an edge of the frame in the map to change the time of the view."
                }


                Heading { text: "Statistics to show" }
                Hint { text: "Choose the lines that each box shows." }
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

                OptionSwitch {
                    key: "rangeSemitones"; text: "Show the Range in semitones (st) and not in Hz"; enabled: config.options.showStats
                    hint: "A semitone is one key on a piano. It is a better unit for a pitch range than Hz."
                }

                ColumnLayout {
                    enabled: config.options.showStats
                    Layout.fillWidth: true
                    spacing: 2
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Window of the statistics" }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [ "The visible window", "The last 5 seconds", "The last 10 seconds", "The last 30 seconds", "The last 60 seconds" ]
                            readonly property var seconds: [0, 5, 10, 30, 60]
                            currentIndex: Math.max(0, seconds.indexOf(config.options.statsWindow))
                            onActivated: config.setOption("statsWindow", seconds[currentIndex])
                        }
                    }
                    Hint { text: "The time that the statistics cover. A short window follows your voice. A long window gives a steadier result." }
                }
                // Space below the last control, so the text does not touch the edge of the window.
                Item { Layout.preferredHeight: ui.space.xxl }
            }
        }

        // Theme
        ScrollView {
            clip: true
            ColumnLayout {
                width: settingsWindow.width - 2 * ui.space.xl
                x: ui.space.xl
                spacing: ui.space.md

                Heading { text: "Color theme"; first: true }
                ComboBox {
                    id: themeBox
                    Layout.fillWidth: true
                    model: config.themeNames()
                    currentIndex: Math.max(0, config.themeNames().indexOf(config.options.theme))
                    onActivated: config.setOption("theme", model[currentIndex])
                }
                OptionSwitch {
                    key: "minVolumeBlack"; text: "Minimum volume is black"
                    hint: "When this is off, the quietest sound has a dark tint of the theme, as in Viridis. A quiet sound then stands out from the black window. When this is on, the quietest sound is black."
                    more: "A quiet sound uses the first color of the theme. Many themes start at black, the color of the window. A quiet sound then has no contrast with the window. When this switch is off, the app lifts the first colors to a dark tint of the theme hue, as the Viridis theme does. The upper colors stay the same. Turn this switch on to get black back."
                }
                Hint { text: "The strip below shows the colors from a quiet sound (left) to a loud sound (right)." }

                // The preview of the color map, from quiet to loud.
                Row {
                    Layout.fillWidth: true
                    height: 36
                    Repeater {
                        model: config.themePreview(config.options.theme, 48)
                        Rectangle {
                            width: (settingsWindow.width - 2 * ui.space.xl) / 48
                            height: 36
                            color: modelData
                        }
                    }
                }

                ConfigSlider {
                    hint: "Lower values make the colors grayer. Higher values make them stronger."
                    title: "Saturation"
                    valueText: Math.round(100 * config.options.colorSaturation) + "%"
                    from: 0.5; to: 1.5; stepSize: 0.05
                    value: config.options.colorSaturation
                    onMoved: (v) => config.setOption("colorSaturation", Math.round(v * 20) / 20)
                }

                // The colors of the tracks.
                RowLayout {
                    Label { text: "Pitch"; color: ui.color.secondary }
                    Rectangle { width: 22; height: 22; radius: 11; color: config.themeColors.pitch; border.color: "black" }
                    Label { text: "Formants"; color: ui.color.secondary; Layout.leftMargin: 12 }
                    Repeater {
                        model: 5
                        Rectangle { width: 22; height: 22; radius: 11; color: config.themeColors.formants[index]; border.color: "black" }
                    }
                }

                Hint {
                    text: "The theme sets the colors of the spectrogram, the pitch track, the formant tracks, and the plots of the oscilloscope and the synthesizer. Every theme works on a dark background. The pride themes use the flag colors at full strength, from dark to light. The brightness of the spectrogram has its own slider (Gain normalization) in the Display tab."
                }

                

                ColumnLayout {
                    visible: config.options.theme === "Custom"
                    Layout.fillWidth: true
                    Heading { text: "Custom theme" }
                    Label { text: "Spectrogram colors, from quiet to loud (#rrggbb, separated by commas)"; color: ui.color.secondary }
                    TextField {
                        Layout.fillWidth: true
                        font.family: ui.font.mono
                        text: config.options.customColors
                        onEditingFinished: config.setOption("customColors", text)
                    }
                    Label { text: "Pitch track color"; color: ui.color.secondary }
                    TextField {
                        Layout.fillWidth: true
                        font.family: ui.font.mono
                        text: config.options.customPitch
                        onEditingFinished: config.setOption("customPitch", text)
                    }
                    Label { text: "Formant track colors (F1 to F5)"; color: ui.color.secondary }
                    TextField {
                        Layout.fillWidth: true
                        font.family: ui.font.mono
                        text: config.options.customFormants
                        onEditingFinished: config.setOption("customFormants", text)
                    }
                }
                // Space below the last control, so the text does not touch the edge of the window.
                Item { Layout.preferredHeight: ui.space.xxl }
            }
        }
    }
}
