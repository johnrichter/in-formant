import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

// A small "?" button. A press opens a box with a longer explanation of a setting. The box scrolls when
// the text is long, and it closes with a press outside it, with the Escape key, or with a second press
// on the button. The short hint under the setting stays. This text adds detail for a person who wants it.
Item {
    id: root

    property string title: ""
    property string text: ""

    implicitWidth: 28
    implicitHeight: 28
    visible: text !== ""

    AppToolButton {
        objectName: "helpButton"
        anchors.fill: parent
        padding: 0
        text: "?"
        font.pixelSize: ui.type.caption
        font.bold: true
        Material.foreground: popup.opened ? "white" : ui.color.secondary
        background: Rectangle {
            radius: ui.radius.md
            color: popup.opened ? Material.color(Material.DeepPurple, Material.ShadeA200) : "transparent"
            border.width: 1
            border.color: popup.opened ? "transparent" : ui.color.tertiary
        }
        onClicked: popup.opened ? popup.close() : popup.open()
        Accessible.name: "More about " + root.title
    }

    Popup {
        id: popup
        parent: root
        // The box ends at the right edge of the button, and it opens below the button.
        x: root.width - width
        y: root.height + ui.space.xs
        width: Math.min(380, (Overlay.overlay ? Overlay.overlay.width : 380) - 2 * ui.space.lg)
        height: Math.min(contentHeight + topPadding + bottomPadding, 260)
        padding: ui.space.md
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Material.roundedScale: Material.SmallScale

        background: Rectangle {
            radius: ui.radius.md
            color: "#2B2930"
            border.width: 1
            border.color: ui.color.tertiary
        }

        contentItem: ScrollView {
            id: scroll
            clip: true
            contentWidth: availableWidth
            // The text decides the height of the content, and the box adds a scroll bar when it is too tall.
            Label {
                width: scroll.availableWidth
                wrapMode: Text.WordWrap
                text: root.text
                color: ui.color.primary
                font.pixelSize: ui.type.body
                lineHeight: ui.type.bodyLine
                lineHeightMode: Text.FixedHeight
            }
        }
    }
}
