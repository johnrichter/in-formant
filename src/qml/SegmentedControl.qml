import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

// A group of choices where exactly one choice is on. The chosen segment has a solid accent fill and
// white bold text. The other segments have an outline and plain text, so each one looks like a button.
Item {
    id: root

    property var model: []
    property int currentIndex: 0
    property int fontSize: ui.type.body
    readonly property color accent: Material.color(Material.DeepPurple, Material.ShadeA200)

    signal activated(int index)

    implicitHeight: 36
    implicitWidth: row.implicitWidth + 2

    Rectangle {
        anchors.fill: parent
        radius: ui.radius.md
        color: "transparent"
        border.width: 2
        border.color: root.accent
    }

    Row {
        id: row
        anchors.fill: parent
        anchors.margins: 2

        Repeater {
            model: root.model

            Rectangle {
                id: segment
                readonly property bool chosen: index === root.currentIndex
                readonly property bool isFirst: index === 0
                readonly property bool isLast: index === root.model.length - 1
                width: label.implicitWidth + 2 * ui.space.lg
                height: row.height
                // Only the outer corners of the group are round. The inner corners are square, so the
                // chosen segment and the hover fill meet their neighbour with no gap.
                topLeftRadius: isFirst ? ui.radius.md - 2 : 0
                bottomLeftRadius: isFirst ? ui.radius.md - 2 : 0
                topRightRadius: isLast ? ui.radius.md - 2 : 0
                bottomRightRadius: isLast ? ui.radius.md - 2 : 0
                color: chosen ? root.accent : (mouse.containsMouse ? "#33FFFFFF" : "transparent")

                Label {
                    id: label
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: root.fontSize
                    font.bold: segment.chosen
                    color: segment.chosen ? "white" : ui.color.primary
                }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.activated(index)
                }

                Accessible.role: Accessible.RadioButton
                Accessible.name: modelData
                Accessible.checked: chosen
            }
        }
    }
}
