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

    font.pixelSize: ui.type.body
    Material.theme: Material.Dark
    // One corner radius for buttons, fields, lists, and popups. It is the medium radius of the design scale.
    Material.roundedScale: Material.SmallScale
    Material.accent: Material.DeepPurple

    // The sheet starts the recording after the person chooses a file name.
    signal start()
    onAccepted: start()

    Component.onCompleted: standardButton(Dialog.Ok).text = "Choose file and record"

    contentItem: ColumnLayout {
        spacing: ui.space.md

        Hint {
            text: "Record writes three files with the same name: a video (.mp4), the sound (.wav) and a report with statistics (.md)."
        }
        OptionSwitch {
            key: "recordVideo"; text: "Write a video"
            hint: "When this is off, Record writes only the sound and the report."
        }
        OptionSlider {
            enabled: config.options.recordVideo
            key: "recordWindow"; title: "Time in the graph of the video"; unit: " s"; from: 2; to: 60; stepSize: 1
            hint: "A short time shows more detail of the pitch and the formants. A long time shows how they change. The graph on the screen uses this time during a recording."
        }
        OptionSlider {
            enabled: config.options.recordVideo
            key: "recordFps"; title: "Frames each second"; from: 10; to: 30; stepSize: 1
            hint: "A higher rate gives a smoother video and a larger file."
        }
        OptionSwitch {
            enabled: config.options.recordVideo
            key: "recordWindows"; text: "Show the open side windows in the video"
            hint: "The open oscilloscope and synthesizer appear in a column at the right of the video."
        }
    }
}
