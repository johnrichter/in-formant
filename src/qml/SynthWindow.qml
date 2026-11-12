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
        if (visible && !sized) {
            sized = true
            width = Math.min(contents.implicitWidth + 4, Screen.desktopAvailableWidth * 0.95)
            height = Math.min(contents.implicitHeight + 4, Screen.desktopAvailableHeight * 0.92)
        }
    }

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

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
                Layout.margins: 10

                Switch {
                    text: "Turn on/off"
                    checked: synth.enabled
                    onToggled: synth.enabled = checked
                }

                MenuSeparator {}

                Label { text: "Aspirate noise volume" }
                Slider {
                    value: synth.noiseGain
                    onMoved: synth.noiseGain = value
                    Layout.topMargin: -10
                    Label {
                        anchors.top: parent.handle.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.handle.horizontalCenter
                        text: Math.round(100 * synth.noiseGain) + "%"
                    }

                }

                MenuSeparator {}

                Label { text: "Glottal source volume" }
                Slider {
                    value: synth.glotGain
                    onMoved: synth.glotGain = value
                    Layout.topMargin: -10
                    Label {
                        anchors.top: parent.handle.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.handle.horizontalCenter
                        text: Math.round(100 * synth.glotGain) + "%"
                    }

                }

                MenuSeparator {}

                Label { text: "Glottal source pitch" }
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
                        text: +synth.glotRd.toFixed(2)
                    }
                }

                MenuSeparator {}
                
                Label {
                    textFormat: Text.RichText
                    text: "Glottal source <var>T<sub>c</sub></var>"
                }
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
                        text: +synth.glotTc.toFixed(2)
                    }
                }

                MenuSeparator {}

                Label { text: "Vocal tract filter shift" }
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
                        text: Math.round(100 * synth.filterShift) + "%"
                    }
                }

                MenuSeparator {}

                Switch {
                    text: "Voicing"
                    checked: synth.voiced
                    onToggled: synth.voiced = checked
                }
            }

            ToolSeparator {
                id: verticalSep
                Layout.fillHeight: true
                Layout.topMargin: 10
                Layout.bottomMargin: 10
            }

            ColumnLayout {
                Layout.margins: 10

                CheckBox {
                    font.italic: true
                    text: "Follow formant estimates"
                    checked: synth.followFormants
                    onToggled: synth.followFormants = checked
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
