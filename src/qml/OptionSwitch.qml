import QtQuick
import QtQuick.Controls

// A switch for one true or false option. The option key names the stored setting.
Switch {
    property string key
    checked: config.options[key]
    onToggled: config.setOption(key, checked)
}
