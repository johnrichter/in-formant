import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

// A tab bar for the tabs of AppTabButton. The chosen tab has its own fill, so the bar has no
// line under the chosen tab.
TabBar {
    id: bar

    contentItem: ListView {
        model: bar.contentModel
        currentIndex: bar.currentIndex
        spacing: bar.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.AutoFlickDirection
        snapMode: ListView.SnapToItem
        highlightMoveDuration: 0
    }
}
