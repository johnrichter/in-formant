import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

// A tab of a tab bar. The chosen tab has a solid accent fill and white bold text. The other tabs have
// plain text and a light fill when the pointer is over them.
TabButton {
    id: tab

    implicitHeight: 40
    font.pixelSize: ui.type.body

    contentItem: Label {
        text: tab.text
        font.pixelSize: tab.font.pixelSize
        font.bold: tab.checked
        color: tab.checked ? "white" : ui.color.primary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        anchors.fill: parent
        anchors.margins: ui.space.xs
        radius: ui.radius.md
        color: tab.checked ? Material.color(Material.DeepPurple, Material.ShadeA200)
                           : (tab.hovered ? "#33FFFFFF" : "transparent")
    }
}
