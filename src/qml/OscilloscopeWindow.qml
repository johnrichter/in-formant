import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import IfCanvas

ApplicationWindow {
    id: scopeWindow
    title: appName + " - Oscilloscope"
    visible: false

    width: 1100
    height: 620
    minimumWidth: 480
    minimumHeight: 320

    font.pixelSize: ui.type.body
    Material.theme: Material.Dark
    // One corner radius for buttons, fields, lists, and popups. It is the medium radius of the design scale.
    Material.roundedScale: Material.SmallScale
    Material.accent: Material.DeepPurple

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ui.space.lg
        spacing: ui.space.md

        IfPlot {
            kind: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 24

            ColumnLayout {
                spacing: 0
                RowLayout {
                    Label { text: "Time in the view" }
                    Slider {
                        from: 5
                        to: 100
                        stepSize: 1
                        value: config.options.scopeWindow
                        onMoved: config.setOption("scopeWindow", value)
                        Layout.preferredWidth: 240
                    }
                    Label {
                        font.family: ui.font.mono
                        text: Math.round(config.options.scopeWindow) + " ms"
                        Layout.minimumWidth: 56
                    }
                }
                Hint {
                    Layout.maximumWidth: 420
                    text: "The length of the sound that the plot shows. A short time shows single cycles of the voice. A long time shows many cycles."
                }
            }

            Item { Layout.fillWidth: true }

            ColumnLayout {
                spacing: 0
                CheckBox {
                    text: "Glottal flow estimate"
                    checked: config.options.scopeShowGif
                    onToggled: config.setOption("scopeShowGif", checked)
                }
                Hint {
                    Layout.maximumWidth: 300
                    text: "A second line with the estimated flow of air through the vocal folds."
                }
            }
        }
    }
}
