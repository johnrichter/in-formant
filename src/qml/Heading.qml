import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// The heading of a group of controls. A large space above it separates the groups, and a small space
// below it joins the heading to its group. The first heading of a page has a smaller space above it.
Label {
    property bool first: false
    Layout.topMargin: first ? ui.space.lg : ui.space.xl
    Layout.bottomMargin: ui.space.xs
    color: ui.color.primary
    font.pixelSize: ui.type.subtitle
    font.bold: true
    lineHeight: ui.type.subtitleLine
    lineHeightMode: Text.FixedHeight
}
