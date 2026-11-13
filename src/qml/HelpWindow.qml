import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// A window with the help pages. Each page is a Markdown file of the resources.
ApplicationWindow {
    id: helpWindow
    title: appName + " - Help"
    visible: false

    width: 760
    height: 720
    minimumWidth: 480
    minimumHeight: 360

    Material.theme: Material.Dark
    // One corner radius for buttons, fields, lists, and popups. It is the medium radius of the design scale.
    Material.roundedScale: Material.SmallScale
    Material.accent: Material.DeepPurple

    readonly property var pages: [
        { title: "Display", file: "reading-the-display" },
        { title: "Settings", file: "choosing-settings" },
        { title: "Feminization", file: "feminization" },
        { title: "Masculinization", file: "masculinization" },
        { title: "Singing", file: "singing" },
        { title: "Warm-ups", file: "warmups-and-vocal-health" },
        { title: "Synthesizer", file: "synthesizer" },
        { title: "The math", file: "the-math" }
    ]

    // Show the window on the page of a file.
    function showPage(file) {
        for (let i = 0; i < pages.length; ++i) {
            if (pages[i].file === file) tabs.currentIndex = i
        }
        show()
        raise()
    }

    header: AppTabBar {
        id: tabs
        objectName: "helpTabs"
        Repeater {
            model: helpWindow.pages
            AppTabButton { text: modelData.title }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        Repeater {
            model: helpWindow.pages

            ScrollView {
                id: scroll
                clip: true
                contentWidth: availableWidth

                // The page is a column of 640 pixels, centred in the window. The typography comes from the prose
                // scale (see gui/uiscale.h): a body of 16 pixels on a line of 28, a large space above each heading,
                // a small space below it, and a gap between two blocks.
                TextEdit {
                    id: page
                    readonly property real column: Math.min(ui.prose.measure, scroll.availableWidth - 2 * ui.prose.pageMargin)
                    x: Math.max(ui.prose.pageMargin, (scroll.availableWidth - column) / 2)
                    y: ui.space.xl
                    width: column
                    readOnly: true
                    selectByMouse: true
                    persistentSelection: true
                    textFormat: TextEdit.MarkdownText
                    wrapMode: TextEdit.WordWrap
                    text: config.helpPage(modelData.file) || "This page is not available."
                    color: ui.color.secondary
                    font.family: ui.font.sans
                    font.pixelSize: ui.prose.bodySize
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                    bottomPadding: ui.space.xxxl

                    Component.onCompleted: helpStyle.apply(page.textDocument, ui.font.sans, ui.font.mono,
                                                           ui.color.primary, ui.color.secondary, ui.color.tertiary,
                                                           "#b39ddb", "#2B2930")

                    HoverHandler { cursorShape: page.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.IBeamCursor }
                }
            }
        }
    }
}
