import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// The choices for one recording. The app stores each choice and shows it again the next time.
Dialog {
    id: sheet
    title: "Record"
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(540, Overlay.overlay ? Overlay.overlay.width - 40 : 540)
    standardButtons: Dialog.Cancel | Dialog.Ok

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    // The sheet starts the recording after the person chooses a file name.
    signal start()
    onAccepted: start()

    Component.onCompleted: standardButton(Dialog.Ok).text = "Choose file and record"

    contentItem: ColumnLayout {
        spacing: 8

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "lightgrey"
            text: "Record writes three files with the same name: a video (.mp4), the sound (.wav) and a report with statistics (.md)."
        }
        OptionSwitch { key: "recordVideo"; text: "Write a video" }
        OptionSlider {
            enabled: config.options.recordVideo
            key: "recordWindow"; title: "Time in the graph of the video"; unit: " s"; from: 2; to: 60; stepSize: 1
        }
        OptionSlider {
            enabled: config.options.recordVideo
            key: "recordFps"; title: "Frames each second"; from: 10; to: 30; stepSize: 1
        }
        OptionSwitch {
            enabled: config.options.recordVideo
            key: "recordWindows"; text: "Show the open side windows in the video"
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "lightgrey"
            text: "A short time shows more detail of the pitch and the formants. A long time shows how they change. The graph on the screen uses this time during a recording."
        }
    }
}
