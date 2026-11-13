import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// A slider for one number option. The label shows the value and the unit.
ColumnLayout {
    property string key
    property string title
    property string unit: ""
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property int decimals: 0
    property string hint: ""
    Layout.fillWidth: true
    spacing: ui.space.xs

    RowLayout {
        Layout.fillWidth: true
        Label { text: title; Layout.fillWidth: true; font.weight: Font.Medium }
        Label { text: Number(config.options[key]).toFixed(decimals) + unit; color: ui.color.secondary; font.family: ui.font.mono }
    }
    Slider {
        Layout.fillWidth: true
        from: parent.from
        to: parent.to
        stepSize: parent.stepSize
        value: config.options[key]
        onMoved: config.setOption(key, value)
    }
    Hint {
        visible: parent.hint !== ""
        text: parent.hint
    }
}
