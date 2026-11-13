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
    Material.accent: Material.DeepPurple

    readonly property var pages: [
        { title: "Display", file: "reading-the-display" },
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

    header: TabBar {
        id: tabs
        objectName: "helpTabs"
        Repeater {
            model: helpWindow.pages
            TabButton { text: modelData.title }
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

                Text {
                    x: 20
                    y: 14
                    width: scroll.availableWidth - 40
                    text: config.helpPage(modelData.file) || "This page is not available."
                    textFormat: Text.MarkdownText
                    wrapMode: Text.WordWrap
                    color: "#e6e6ea"
                    linkColor: "#b39ddb"
                    font.pixelSize: 15
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                    bottomPadding: 28
                }
            }
        }
    }
}
