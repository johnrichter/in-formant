import QtQuick 
import QtQuick.Layouts 
import QtQuick.Controls 
import QtQuick.Controls.Material 
import IfCanvas

ApplicationWindow {
    id: synthWindow
    title: appName + " - Synthesizer"
    visible: false

    width: 400
    height: 300

    // The first time the window opens, it takes the size of its contents.
    property bool sized: false
    onVisibleChanged: {
        // The synthesizer makes sound only while its window is open. A closed window turns it off.
        if (!visible && synth.enabled) {
            synth.enabled = false
        }
        if (visible && !sized) {
            sized = true
            width = Math.min(contents.implicitWidth + 4, Screen.desktopAvailableWidth * 0.95)
            height = Math.min(contents.implicitHeight + 4, Screen.desktopAvailableHeight * 0.92)
        }
    }

    font.pixelSize: ui.type.body
    Material.theme: Material.Dark
    // One corner radius for buttons, fields, lists, and popups. It is the medium radius of the design scale.
    Material.roundedScale: Material.SmallScale
    Material.accent: Material.DeepPurple

    // The first three formants of ten vowels for an average adult man and an average adult woman (Peterson and
    // Barney, 1952, American English). F4 is a typical value, because the study has no F4.
    readonly property var vowels: [
        { name: "ee, as in see",      male: [270, 2290, 3010], female: [310, 2790, 3310] },
        { name: "ih, as in sit",      male: [390, 1990, 2550], female: [430, 2480, 3070] },
        { name: "eh, as in bed",      male: [530, 1840, 2480], female: [610, 2330, 2990] },
        { name: "ae, as in cat",      male: [660, 1720, 2410], female: [860, 2050, 2850] },
        { name: "ah, as in father",   male: [730, 1090, 2440], female: [850, 1220, 2810] },
        { name: "aw, as in thought",  male: [570, 840, 2410],  female: [590, 920, 2710] },
        { name: "uh, as in book",     male: [440, 1020, 2240], female: [470, 1160, 2680] },
        { name: "oo, as in food",     male: [300, 870, 2240],  female: [370, 950, 2670] },
        { name: "u, as in cup",       male: [640, 1190, 2390], female: [760, 1400, 2780] },
        { name: "er, as in bird",     male: [490, 1350, 1690], female: [500, 1640, 1960] }
    ]
    readonly property var voices: [
        { name: "Lower voice (average man)", key: "male", pitch: 120, f4: 3500 },
        { name: "Higher voice (average woman)", key: "female", pitch: 210, f4: 4000 }
    ]
    readonly property var defaultBandwidths: [60, 90, 120, 150]

    // Set the four formants to one vowel, and optionally the pitch. The follow modes go off, because they
    // would replace the values at once.
    function applyPreset() {
        const voice = voices[voiceBox.currentIndex]
        const set = vowels[vowelBox.currentIndex][voice.key]
        synth.followFormants = false
        for (let i = 0; i < 3; ++i) {
            synth.setFormantFrequency(i, set[i])
        }
        synth.setFormantFrequency(3, voice.f4)
        for (let i = 0; i < 4; ++i) {
            synth.setFormantBandwidth(i, defaultBandwidths[i])
        }
        if (presetPitch.checked) {
            synth.followPitch = false
            synth.glotPitch = voice.pitch
        }
    }

    // Return every control to the value that the program has at the start.
    function resetAll() {
        synth.followPitch = false
        synth.followFormants = false
        synth.noiseGain = 0.3
        synth.glotGain = 1.0
        synth.glotPitch = 170
        synth.glotRd = 1.7
        synth.glotTc = 1.0
        synth.filterShift = 1.0
        synth.voiced = true
        synth.jitter = 0.0
        const neutral = [500, 1500, 2500, 3500]
        for (let i = 0; i < 4; ++i) {
            synth.setFormantFrequency(i, neutral[i])
            synth.setFormantBandwidth(i, defaultBandwidths[i])
        }
    }

    // A short grey line under a control.
    component SmallHint: Label {
        Layout.fillWidth: true
        Layout.maximumWidth: 300
        wrapMode: Text.WordWrap
        color: ui.color.secondary
        font.pixelSize: ui.type.caption
        lineHeight: ui.type.captionLine
        lineHeightMode: Text.FixedHeight
    }

    Flickable {
        id: container
        width: synthWindow.width
        height: synthWindow.height
        contentWidth: contents.width
        contentHeight: contents.height

        ScrollBar.vertical: ScrollBar {}
        
        RowLayout {
            id: contents
            width: Math.max(implicitWidth, synthWindow.width)
            height: Math.max(implicitHeight, synthWindow.height)

            ColumnLayout {
                Layout.margins: ui.space.lg
                spacing: ui.space.sm

                Switch {
                    text: "Turn on/off"
                    checked: synth.enabled
                    onToggled: synth.enabled = checked
                }
                SmallHint { text: "Use headphones. The microphone can hear the speakers and change the sound."; Layout.topMargin: ui.space.xs }

                MenuSeparator {}

                Label { text: "Aspirate noise volume" }
                SmallHint { text: "Breath noise. 0% is a clean voice. 30% is a little breathy. 100% is very breathy." }
                Slider {
                    value: synth.noiseGain
                    onMoved: synth.noiseGain = value
                    Layout.topMargin: -10
                    Label {
                        anchors.top: parent.handle.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.handle.horizontalCenter
                        font.family: ui.font.mono
                        text: Math.round(100 * synth.noiseGain) + "%"
                    }

                }

                MenuSeparator {}

                Label { text: "Glottal source volume" }
                SmallHint { text: "The loudness of the buzz of the vocal folds. Keep it at 100% for a normal voice." }
                Slider {
                    value: synth.glotGain
                    onMoved: synth.glotGain = value
                    Layout.topMargin: -10
                    Label {
                        anchors.top: parent.handle.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.handle.horizontalCenter
                        font.family: ui.font.mono
                        text: Math.round(100 * synth.glotGain) + "%"
                    }

                }

                MenuSeparator {}

                Label { text: "Glottal source pitch" }
                SmallHint { text: "How high the voice is. About 120 Hz is a low voice, 170 Hz is in the middle, and 210 Hz is a high voice." }
                SpinBox {
                    from: 60
                    to: 1500
                    stepSize: 5
                    editable: true
                    value: Math.round(synth.glotPitch)
                    font.family: ui.font.mono
                    textFromValue: (v) => v + " Hz"
                    valueFromText: (t) => parseInt(t)
                    enabled: !synth.followPitch
                    onValueModified: synth.glotPitch = value
                }
                Slider {
                    from: mel(60)
                    to: mel(1500)
                    value: mel(synth.glotPitch)
                    onMoved: synth.glotPitch = hz(value)
                    Layout.topMargin: -10
                    Label {
                        anchors.top: parent.handle.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.handle.horizontalCenter
                        font.family: ui.font.mono
                        text: Math.round(synth.glotPitch) + " Hz"
                    }
                }

                CheckBox {
                    font.italic: true
                    text: "Follow pitch estimates"
                    checked: synth.followPitch
                    onToggled: synth.followPitch = checked
                }

                MenuSeparator {}

                Label {
                    textFormat: Text.RichText
                    text: "Glottal source <var>R<sub>d</sub></var>"
                }
                SmallHint { text: "How soft the buzz is. A low value (0.7) is pressed, bright, and loud. A high value (2.6) is soft, breathy, and dark." }
                Slider {
                    from: 0.7
                    to: 2.6
                    value: synth.glotRd
                    onMoved: synth.glotRd = value
                    Layout.topMargin: -10
                    Label {
                        anchors.top: parent.handle.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.handle.horizontalCenter
                        font.family: ui.font.mono
                        text: +synth.glotRd.toFixed(2)
                    }
                }

                MenuSeparator {}
                
                Label {
                    textFormat: Text.RichText
                    text: "Glottal source <var>T<sub>c</sub></var>"
                }
                SmallHint { text: "The share of each cycle in which the vocal folds move. At 1.0 the folds never rest. A lower value adds a short rest. Small steps only." }
                Slider {
                    from: 0.9
                    to: 1.0
                    value: synth.glotTc
                    onMoved: synth.glotTc = value
                    Layout.topMargin: -10
                    Label {
                        anchors.top: parent.handle.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.handle.horizontalCenter
                        font.family: ui.font.mono
                        text: +synth.glotTc.toFixed(2)
                    }
                }

                MenuSeparator {}

                Label { text: "Vocal tract filter shift" }
                SmallHint { text: "Moves all formants up or down at one time. Keep it at 100% when you set the formants yourself." }
                Slider {
                    from: 0.2
                    to: 2.0
                    value: synth.filterShift
                    onMoved: synth.filterShift = value
                    Layout.topMargin: -10
                    Label {
                        anchors.top: parent.handle.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.handle.horizontalCenter
                        font.family: ui.font.mono
                        text: Math.round(100 * synth.filterShift) + "%"
                    }
                }

                MenuSeparator {}

                Switch {
                    text: "Voicing"
                    checked: synth.voiced
                    onToggled: synth.voiced = checked
                }
                SmallHint { text: "Off removes the buzz, and only the breath noise stays. This makes a whisper."; Layout.topMargin: ui.space.xs }

                MenuSeparator {}

                Label { text: "Pitch jitter" }
                SmallHint { text: "Small random changes of the pitch. 0% is a perfect machine voice. A few percent sounds more human." }
                Slider {
                    from: 0
                    to: 1
                    value: synth.jitter
                    onMoved: synth.jitter = value
                    Layout.topMargin: -10
                    Label {
                        anchors.top: parent.handle.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.handle.horizontalCenter
                        font.family: ui.font.mono
                        text: Math.round(100 * synth.jitter) + "%"
                    }
                }
            }

            ToolSeparator {
                id: verticalSep
                Layout.fillHeight: true
                Layout.topMargin: 10
                Layout.bottomMargin: 10
            }

            ColumnLayout {
                Layout.margins: ui.space.lg
                spacing: ui.space.sm

                Heading { text: "Quick start"; first: true }
                Label {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 520
                    wrapMode: Text.WordWrap
                    color: ui.color.secondary
                    text: "1. Wear headphones.  2. Choose a vowel and a voice.  3. Press Apply.  4. Start the synthesizer with the switch at the top left.  5. Change one control at a time, and listen after each change."
                }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox {
                        id: vowelBox
                        Layout.fillWidth: true
                        implicitContentWidthPolicy: ComboBox.WidestText
                        model: synthWindow.vowels.map(v => v.name)
                        currentIndex: 4
                    }
                    ComboBox {
                        id: voiceBox
                        Layout.fillWidth: true
                        implicitContentWidthPolicy: ComboBox.WidestText
                        model: synthWindow.voices.map(v => v.name)
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        id: presetPitch
                        text: "Also set the pitch"
                        checked: true
                    }
                    Item { Layout.fillWidth: true }
                    AppButton { text: "Apply"; highlighted: true; onClicked: synthWindow.applyPreset() }
                    AppButton { text: "Reset all"; onClicked: synthWindow.resetAll() }
                    AppButton { text: "How to use"; onClicked: helpWindow.showPage("synthesizer") }
                }

                MenuSeparator { Layout.fillWidth: true }

                CheckBox {
                    font.italic: true
                    text: "Follow formant estimates"
                    checked: synth.followFormants
                    onToggled: synth.followFormants = checked
                }

                // The microphone hears the speaker in a follow mode. The estimates then follow the
                // sound of the synthesizer, and the sound changes by itself.
                Label {
                    visible: synth.enabled && (synth.followPitch || synth.followFormants)
                    Layout.fillWidth: true
                    Layout.preferredWidth: 520
                    wrapMode: Text.WordWrap
                    color: Material.color(Material.Orange, Material.Shade300)
                    text: "A follow mode is on. Use headphones. Without headphones, the microphone hears the synthesizer and the sound changes by itself."
                }

                Label {
                    text: "Formants (used when follow formants is off)"
                    color: ui.color.secondary
                }

                GridLayout {
                    columns: 5
                    columnSpacing: 8
                    rowSpacing: 0

                    Repeater {
                        model: 4

                        Label {
                            Layout.row: index
                            Layout.column: 0
                            text: "F" + (index + 1)
                        }
                    }

                    Repeater {
                        model: 4

                        Slider {
                            Layout.row: index
                            Layout.column: 1
                            Layout.fillWidth: true
                            from: mel(150)
                            to: mel(5500)
                            value: mel(synth.formantFrequencies[index])
                            onMoved: synth.setFormantFrequency(index, hz(value))
                            enabled: !synth.followFormants
                        }
                    }

                    // A box for each formant. Click the number and type a value, or use the arrows.
                    Repeater {
                        model: 4

                        SpinBox {
                            Layout.row: index
                            Layout.column: 2
                            Layout.preferredWidth: 180
                            from: 150
                            to: 5500
                            stepSize: 10
                            editable: true
                            value: Math.round(synth.formantFrequencies[index])
                            font.family: ui.font.mono
                    textFromValue: (v) => v + " Hz"
                            valueFromText: (t) => parseInt(t)
                            enabled: !synth.followFormants
                            onValueModified: synth.setFormantFrequency(index, value)
                        }
                    }

                    Repeater {
                        model: 4

                        Slider {
                            Layout.row: index
                            Layout.column: 3
                            Layout.preferredWidth: 120
                            from: 20
                            to: 300
                            value: synth.formantBandwidths[index]
                            onMoved: synth.setFormantBandwidth(index, value)
                        }
                    }

                    Repeater {
                        model: 4

                        Label {
                            Layout.row: index
                            Layout.column: 4
                            Layout.preferredWidth: 90
                            horizontalAlignment: Text.AlignRight
                            font.family: ui.font.mono
                            text: "bw " + Math.round(synth.formantBandwidths[index]) + " Hz"
                        }
                    }
                }

                IfPlot {
                    kind: 1
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 520
                    Layout.preferredHeight: 240
                    Layout.minimumHeight: 140
                }

                MenuSeparator {}

                RowLayout {
                    IfPlot {
                        kind: 2
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 255
                        Layout.preferredHeight: 200
                        Layout.minimumHeight: 120
                    }

                    IfPlot {
                        kind: 3
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 255
                        Layout.preferredHeight: 200
                        Layout.minimumHeight: 120
                    }
                }
            }
        }
    }
}
