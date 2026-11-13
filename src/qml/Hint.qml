import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// A short line of text under a control. It says what the control does. The size, the line height, the
// colour, and the longest line come from the scale (see gui/uiscale.h).
Label {
    Layout.fillWidth: true
    Layout.maximumWidth: ui.measure
    wrapMode: Text.WordWrap
    color: ui.color.secondary
    font.pixelSize: ui.type.caption
    lineHeight: ui.type.captionLine
    lineHeightMode: Text.FixedHeight
}
