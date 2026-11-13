import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import IfCanvas 

ApplicationWindow {
    id: mainWindow
    title: appName
    visible: false

    width: 1600
    height: 900

    Material.theme: Material.Dark
    // One corner radius for buttons, fields, lists, and popups. It is the medium radius of the design scale.
    Material.roundedScale: Material.SmallScale
    Material.accent: Material.DeepPurple

    // The synthesizer window uses these two functions for its pitch slider.
    function mel(f) {
        return 2595 * Math.log10(1 + f / 700);
    }

    function hz(m) {
        return 700 * (Math.pow(10, m / 2595) - 1);
    }

    Shortcut {
        sequence: "M"
        enabled: practice.active && !noteField.activeFocus
        onActivated: practice.mark("")
    }

    Shortcut {
        sequence: "Space"
        onActivated: config.paused = !config.paused
    }

    // Command and comma on a Mac. This opens the one window for all settings.
    Shortcut {
        sequences: [StandardKey.Preferences]
        onActivated: settingsWindow.show()
    }

    header: ToolBar {
        Material.background: Material.color(Material.BlueGrey, Material.Shade800)

        RowLayout {
            anchors.fill: parent

            RowLayout {
                Layout.alignment: Qt.AlignLeft

                spacing: 0

                RowLayout {
                    Layout.fillHeight: true

                    id: zoomPanel

                    spacing: -2

                    AppToolButton {
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "qrc:/icons/zoom_out.svg"
                        onPressed: if (config.viewZoom <= 2.0) config.viewZoom -= 0.1; else config.viewZoom -= 0.5
                        enabled: config.viewZoom >= 0.6
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: (Math.round(100 * config.viewZoom)) + "%"
                        color: ui.color.secondary
                    }

                    AppToolButton {
                        Layout.alignment: Qt.AlignRight
                        icon.source: "qrc:/icons/zoom_in.svg"
                        onPressed: if (config.viewZoom < 2.0) config.viewZoom += 0.1; else config.viewZoom += 0.5
                        enabled: config.viewZoom < 5.0
                    }
                }

                // The graph shows the spectrogram and all tracks, or the pitch track alone.
                SegmentedControl {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                    model: [ "Full", "Pitch only" ]
                    currentIndex: config.options.graphMode === "pitch" ? 1 : 0
                    onActivated: (index) => config.setOption("graphMode", index === 1 ? "pitch" : "full")
                }

                AppToolButton {
                    icon.source: config.paused ? "qrc:/icons/play_arrow.svg" : "qrc:/icons/stop.svg"
                    checked: config.paused 
                    onPressed: config.paused = !config.paused 
                    
                    Component.onCompleted: {
                        icon.width = 2 * icon.width
                        icon.height = 2 * icon.height
                    }
                }
            }
            
            RowLayout {
                Layout.alignment: Qt.AlignRight

                spacing: 1

                AppButton {
                    text: "Help"
                    Material.background: Material.color(Material.BlueGrey, Material.Shade600)
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 10
                    onPressed: helpWindow.show()
                }

                AppButton {
                    text: "Progress"
                    Material.background: Material.color(Material.BlueGrey, Material.Shade600)
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 10
                    onPressed: progressWindow.show()
                }

                AppButton {
                    text: "Settings"
                    Material.background: Material.color(Material.BlueGrey, Material.Shade600)
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 10
                    onPressed: settingsWindow.show()
                }

                AppButton {
                    text: "Oscilloscope"
                    Material.background: Material.color(Material.DeepPurple, Material.ShadeA200)
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 10
                    onPressed: oscilloscopeWindow.show()
                } 

                AppButton {
                    visible: HAS_SYNTH
                    text: "Synthesizer"
                    Material.background: Material.color(Material.DeepPurple, Material.ShadeA200)
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 10
                    onPressed: synthWindow.show()
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    IfCanvas {
        id: canvas
        anchors.top: parent.top
        anchors.bottom: overviewBar.top
        anchors.right: parent.right

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: (event) => sessionControl.scrollView((config.options.flipTime ? 1 : -1) * (Math.abs(event.angleDelta.x) > Math.abs(event.angleDelta.y) ? event.angleDelta.x : event.angleDelta.y) / 120.0)
        }

        // The application starts with the capture off. This hint shows on the empty graph.
        Label {
            id: playHint
            anchors.centerIn: parent
            text: "Press play to start"
            color: ui.color.secondary
            font.pixelSize: ui.type.headline
            property bool noSound: true
            visible: config.paused && !sessionControl.fileMode && noSound

            Timer {
                interval: 250
                repeat: true
                running: config.paused
                triggeredOnStart: true
                onTriggered: playHint.noSound = sessionControl.duration() <= 0
            }
        }

        // A click or a drag on the graph picks a moment. A double click or a right click clears it.
        TapHandler {
            acceptedButtons: Qt.LeftButton
            onTapped: (point) => config.pickMoment(point.position.x / canvas.width, point.position.y / canvas.height)
            onDoubleTapped: config.clearMoment()
        }

        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: config.clearMoment()
        }

        DragHandler {
            target: null
            acceptedButtons: Qt.LeftButton
            onCentroidChanged: if (active) config.pickMoment(centroid.position.x / canvas.width, 0.0)
        }

        width: parent.width
    }

    // The map of the whole session, and the controls for the playback and the export.
    Rectangle {
        id: overviewBar
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: canvas.width
        height: config.options.showMiniMap ? 112 : 34
        color: "#101116"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                Layout.leftMargin: 6
                Layout.rightMargin: 6
                spacing: 4

                component BarButton: AppButton {
                    flat: true
                    font.pixelSize: ui.type.caption
                    implicitHeight: 30
                    topInset: 0
                    bottomInset: 0
                }

                BarButton {
                    text: "Live"
                    enabled: !sessionControl.recording && (!config.viewLive || sessionControl.playing || sessionControl.fileMode)
                    onClicked: sessionControl.goLive()
                }

                BarButton {
                    text: "Open file"
                    enabled: !sessionControl.recording
                    onClicked: sessionControl.openFile()
                }

                BarButton {
                    text: sessionControl.recording ? "Stop recording " + sessionControl.recordTime : "Record"
                    enabled: !sessionControl.fileMode && !practice.active
                    Material.foreground: sessionControl.recording ? Material.color(Material.Red, Material.Shade300) : undefined
                    onClicked: sessionControl.recording ? sessionControl.toggleRecording() : recordSheet.open()
                }

                BarButton {
                    text: sessionControl.playing ? "Stop" : "Play window"
                    enabled: !sessionControl.recording
                    onClicked: sessionControl.playing ? sessionControl.stopPlayback() : sessionControl.playWindow()
                }

                BarButton {
                    text: "Export window"
                    enabled: !sessionControl.exporting
                    onClicked: sessionControl.exportWindow()
                }

                BarButton {
                    text: "Export all"
                    enabled: !sessionControl.exporting
                    onClicked: sessionControl.exportAll()
                }

                // The practice session: one click starts it, and one click stops it. The M key and the Mark button save marks.
                BarButton {
                    text: practice.active ? "Stop practice " + practice.elapsed : "Practice"
                    enabled: practice.active || (!sessionControl.recording && !sessionControl.fileMode)
                    Material.foreground: practice.active ? Material.color(Material.Red, Material.Shade300) : undefined
                    onClicked: practice.active ? practice.stop() : practice.start()
                }

                ComboBox {
                    id: taskBox
                    // The box is as wide as its longest entry, so "Conversation" shows in full.
                    implicitContentWidthPolicy: ComboBox.WidestText
                    Layout.preferredWidth: implicitWidth
                    implicitHeight: 30
                    font.pixelSize: ui.type.caption
                    topInset: 0
                    bottomInset: 0
                    model: practice.tags
                    currentIndex: Math.max(0, practice.tags.indexOf(practice.tag))
                    onActivated: practice.tag = model[currentIndex]
                }

                BarButton {
                    visible: practice.active
                    text: "Mark (" + practice.markCount + ")"
                    onClicked: {
                        practice.mark(noteField.text)
                        noteField.text = ""
                    }
                }

                TextField {
                    id: noteField
                    visible: practice.active
                    Layout.preferredWidth: 110
                    implicitHeight: 28
                    font.pixelSize: ui.type.caption
                    topPadding: 0
                    bottomPadding: 0
                    topInset: 0
                    bottomInset: 0
                    verticalAlignment: Text.AlignVCenter
                    placeholderText: "Note, Enter marks"
                    onAccepted: {
                        practice.mark(text)
                        text = ""
                        focus = false
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: sessionControl.status
                    color: ui.color.secondary
                    font.pixelSize: ui.type.caption
                    elide: Text.ElideRight
                }

                // The time of the view. The map below this bar shows the same time in its frame.
                Label { text: "View"; color: ui.color.secondary; font.pixelSize: ui.type.caption }
                TextField {
                    id: timeSpanField
                    Layout.preferredWidth: 72
                    implicitHeight: 28
                    font.pixelSize: ui.type.caption
                    font.family: ui.font.mono
                    topPadding: 0
                    bottomPadding: 0
                    topInset: 0
                    bottomInset: 0
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    validator: DoubleValidator { bottom: 0.5; top: 86400; decimals: 1; notation: DoubleValidator.StandardNotation }
                    clip: true
                    text: String(Math.round(config.viewTimeSpan * 10) / 10)
                    onEditingFinished: {
                        const v = parseFloat(text)
                        if (!isNaN(v)) config.viewTimeSpan = Math.min(Math.max(v, 0.5), 86400)
                        // Show the stored value again, also when the input was out of range.
                        text = Qt.binding(function() { return String(Math.round(config.viewTimeSpan * 10) / 10) })
                        focus = false
                    }
                }
                Label { text: "s"; color: ui.color.secondary; font.pixelSize: ui.type.caption }
            }

            IfOverview {
                visible: config.options.showMiniMap
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    property SynthWindow synthWindow

    OscilloscopeWindow {
        id: oscilloscopeWindow
    }

    SettingsWindow {
        id: settingsWindow
    }

    RecordSheet {
        id: recordSheet
        objectName: "recordSheet"
        onStart: sessionControl.toggleRecording()
    }

    HelpWindow {
        id: helpWindow
    }

    ProgressWindow {
        id: progressWindow
    }

    // The question after a practice session: keep it or discard it. The text shows the length, the marks, and
    // the comparison with the last session of the same task. The person must choose, so no other action closes it.
    Dialog {
        id: practiceDone
        Material.roundedScale: Material.SmallScale
        objectName: "practiceDone"
        title: "Keep this practice session?"
        modal: true
        closePolicy: Dialog.NoAutoClose
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Save | Dialog.Discard
        property string text: ""

        Component.onCompleted: {
            standardButton(Dialog.Save).text = "Keep"
            standardButton(Dialog.Discard).text = "Discard"
        }
        onAccepted: practice.keep()
        onDiscarded: practice.discard()

        ColumnLayout {
            spacing: ui.space.md
            Label {
                Layout.preferredWidth: 520
                wrapMode: Text.WordWrap
                text: practiceDone.text
            }
            Hint {
                text: "Keep saves the statistics, the marks, a report, and the sound in the folder of the practice sessions. Discard saves nothing."
            }
        }
    }

    Connections {
        target: practice
        function onConfirm(text) {
            practiceDone.text = text
            practiceDone.open()
        }
    }

    Component.onCompleted: {
        if (HAS_SYNTH) {
            synthWindow = Qt.createQmlObject('SynthWindow {}', mainWindow);
        }
    }
}

