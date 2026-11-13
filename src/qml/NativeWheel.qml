import QtQuick

// Scrolls a Flickable with the wheel or the trackpad, at the rate of the operating system.
//
// The Flickable of Qt ignores the pixel distance that the system reports. It converts the angle of the wheel
// to its own flick, and a trackpad moves the content by about one pixel for a swipe of 200 pixels. This handler
// follows the same rules as a native scroll view:
// - A trackpad or a precise wheel reports a distance in pixels. The system has already applied the speed that
//   the user chose, the acceleration, and the momentum at the end of a swipe. The content moves by that distance.
// - A wheel with notches reports an angle. A notch is 120 units. The content moves by the number of lines of the
//   system (Qt.styleHints.wheelScrollLines) times the scroll line of the scale, for each notch.
// - The direction follows the system, so the setting for natural scrolling applies.
//
// Put the handler inside the Flickable, or inside a ScrollView, and set the property flickable.
WheelHandler {
    id: handler

    // The Flickable to move. A ScrollView gives its Flickable in the property contentItem.
    property Flickable flickable: null

    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

    onWheel: (event) => {
        const view = handler.flickable
        if (!view) return

        // The distance of the event, in pixels. Positive means that the content moves down.
        let distance = event.pixelDelta.y
        if (distance === 0) distance = event.angleDelta.y / 120 * Qt.styleHints.wheelScrollLines * ui.scrollLine

        const top = view.originY - view.topMargin
        const bottom = view.originY + Math.max(0, view.contentHeight + view.bottomMargin - view.height)
        view.cancelFlick()
        view.contentY = Math.max(top, Math.min(bottom, view.contentY - distance))
        event.accepted = true
    }
}
