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
    Material.accent: Material.DeepPurple

    // The synthesizer window uses these two functions for its pitch slider.
    function mel(f) {
        return 2595 * Math.log10(1 + f / 700);
    }

    function hz(m) {
        return 700 * (Math.pow(10, m / 2595) - 1);
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

                    ToolButton {
                        Layout.alignment: Qt.AlignLeft
                        icon.source: "qrc:/icons/zoom_out.svg"
                        onPressed: if (config.viewZoom <= 2.0) config.viewZoom -= 0.1; else config.viewZoom -= 0.5
                        enabled: config.viewZoom >= 0.6
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: (Math.round(100 * config.viewZoom)) + "%"
                        color: "lightgrey"
                    }

                    ToolButton {
                        Layout.alignment: Qt.AlignRight
                        icon.source: "qrc:/icons/zoom_in.svg"
                        onPressed: if (config.viewZoom < 2.0) config.viewZoom += 0.1; else config.viewZoom += 0.5
                        enabled: config.viewZoom < 5.0
                    }
                }

                ToolButton {
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

                Button {
                    text: "Help"
                    Material.background: Material.color(Material.BlueGrey, Material.Shade600)
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 10
                    onPressed: helpWindow.show()
                }

                Button {
                    text: "Settings"
                    Material.background: Material.color(Material.BlueGrey, Material.Shade600)
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 10
                    onPressed: settingsWindow.show()
                }

                Button {
                    text: "Oscilloscope"
                    Material.background: Material.color(Material.DeepPurple, Material.ShadeA200)
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 10
                    onPressed: oscilloscopeWindow.show()
                } 

                Button {
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
            onWheel: (event) => sessionControl.scrollView(-(Math.abs(event.angleDelta.x) > Math.abs(event.angleDelta.y) ? event.angleDelta.x : event.angleDelta.y) / 120.0)
        }

        // The application starts with the capture off. This hint shows on the empty graph.
        Label {
            id: playHint
            anchors.centerIn: parent
            text: "Press play to start"
            color: "lightgrey"
            font.pixelSize: 24
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

                component BarButton: Button {
                    flat: true
                    font.pixelSize: 12
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
                    enabled: !sessionControl.fileMode
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

                Label {
                    Layout.fillWidth: true
                    text: sessionControl.status
                    color: "lightgrey"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                // The time of the view. The map below this bar shows the same time in its frame.
                Label { text: "View"; color: "lightgrey"; font.pixelSize: 12 }
                TextField {
                    id: timeSpanField
                    Layout.preferredWidth: 72
                    implicitHeight: 28
                    font.pixelSize: 12
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
                Label { text: "s"; color: "lightgrey"; font.pixelSize: 12 }
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

    Component.onCompleted: {
        if (HAS_SYNTH) {
            synthWindow = Qt.createQmlObject('SynthWindow {}', mainWindow);
        }
    }
}

