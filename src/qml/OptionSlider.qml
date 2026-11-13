import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// A slider for one number option. The label shows the value and the unit.
ColumnLayout {
    id: os
    property string key
    property string title
    property string unit: ""
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property int decimals: 0
    // A factor between the stored value and the shown value. A value of 100 shows a fraction as a percent.
    property real displayScale: 1
    property string hint: ""
    property string more: ""
    Layout.fillWidth: true
    spacing: ui.space.xs

    RowLayout {
        Layout.fillWidth: true
        Label { text: title; Layout.fillWidth: true; font.weight: Font.Medium }
        Label { text: (Number(config.options[key]) * displayScale).toFixed(decimals) + unit; color: ui.color.secondary; font.family: ui.font.mono }
        HelpButton { title: os.title; text: os.more }
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
