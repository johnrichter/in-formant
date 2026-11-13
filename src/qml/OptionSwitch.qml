import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// A switch for one true or false option, with an optional grey hint under it. The option key names the stored setting.
ColumnLayout {
    id: root
    property string key
    property string text
    property string hint
    Layout.fillWidth: true
    // A switch is a tall control, so its hint needs more room than the hint of a slider.
    spacing: ui.space.sm

    Switch {
        text: root.text
        checked: config.options[root.key]
        onToggled: config.setOption(root.key, checked)
    }
    Hint {
        visible: root.hint !== ""
        text: root.hint
        Layout.leftMargin: ui.space.xs
    }
}
