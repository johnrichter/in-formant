import QtQuick
import QtQuick.Controls

// A ScrollView that scrolls at the rate of the operating system. See NativeWheel.qml.
ScrollView {
    id: root

    NativeWheel { flickable: root.contentItem as Flickable }
}
