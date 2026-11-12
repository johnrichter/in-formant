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

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        IfPlot {
            kind: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true

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
                text: Math.round(config.options.scopeWindow) + " ms"
                Layout.minimumWidth: 56
            }

            Item { Layout.fillWidth: true }

            CheckBox {
                text: "Glottal flow estimate"
                checked: config.options.scopeShowGif
                onToggled: config.setOption("scopeShowGif", checked)
            }
        }
    }
}
