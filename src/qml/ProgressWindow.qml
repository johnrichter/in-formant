import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// The trends of the practice sessions: a chart and a table for a measure, a task, and a period.
ApplicationWindow {
    id: progressWindow
    title: appName + " - Progress"
    visible: false

    width: 820
    height: 700
    minimumWidth: 520
    minimumHeight: 420

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    readonly property var measures: [
        { name: "Median pitch", key: "pitch", unit: "Hz", digits: 1 },
        { name: "Median F1", key: "f1", unit: "Hz", digits: 0 },
        { name: "Median F2", key: "f2", unit: "Hz", digits: 0 },
        { name: "Median F3", key: "f3", unit: "Hz", digits: 0 },
        { name: "Median F4", key: "f4", unit: "Hz", digits: 0 },
        { name: "Pitch range (5th to 95th percentile)", key: "range", unit: "semitones", digits: 1 },
        { name: "Pitch variation (SD divided by mean)", key: "variation", unit: "%", digits: 1 }
    ]
    readonly property var periods: [
        { name: "Day", key: "day" },
        { name: "Week", key: "week" },
        { name: "Month", key: "month" },
        { name: "Year", key: "year" }
    ]

    property var points: []
    property var tagNames: ["All tasks"]
    readonly property var measure: measures[measureBox.currentIndex]

    function refresh() {
        const names = practice.libraryTags()
        const keep = taskBox.currentText
        tagNames = ["All tasks"].concat(names)
        // The task of the last practice is the default filter, because a formant over mixed tasks says little.
        const wanted = keep && tagNames.indexOf(keep) >= 0 ? keep : (tagNames.indexOf(practice.tag) >= 0 ? practice.tag : "All tasks")
        taskBox.currentIndex = Math.max(0, tagNames.indexOf(wanted))
        points = practice.trend(measure.key, periods[periodBox.currentIndex].key, taskBox.currentIndex > 0 ? tagNames[taskBox.currentIndex] : "")
        chart.requestPaint()
    }

    function formatValue(v) {
        return Number(v).toFixed(measure.digits)
    }

    onVisibleChanged: if (visible) refresh()

    Connections {
        target: practice
        function onSaved() { if (progressWindow.visible) progressWindow.refresh() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ComboBox {
                id: measureBox
                Layout.fillWidth: true
                model: progressWindow.measures.map(m => m.name)
                onActivated: progressWindow.refresh()
            }
            ComboBox {
                id: taskBox
                Layout.preferredWidth: 150
                model: progressWindow.tagNames
                onActivated: progressWindow.refresh()
            }
            ComboBox {
                id: periodBox
                Layout.preferredWidth: 110
                model: progressWindow.periods.map(p => p.name)
                currentIndex: 1
                onActivated: progressWindow.refresh()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: progressWindow.points.length === 0
            wrapMode: Text.WordWrap
            color: "lightgrey"
            text: "No session has a value for this choice. Practice with the Practice button, or choose another task or measure."
        }

        Canvas {
            id: chart
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            visible: progressWindow.points.length > 0

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                const pts = progressWindow.points
                if (pts.length === 0) return

                const left = 64, right = 16, top = 14, bottom = 34
                const w = width - left - right
                const h = height - top - bottom
                let lo = Infinity, hi = -Infinity
                for (const p of pts) {
                    lo = Math.min(lo, p.min)
                    hi = Math.max(hi, p.max)
                }
                if (hi - lo < 1e-6) { lo -= 1; hi += 1 }
                const pad = 0.08 * (hi - lo)
                lo -= pad
                hi += pad

                const xAt = i => left + (pts.length === 1 ? 0.5 * w : (w * i) / (pts.length - 1))
                const yAt = v => top + h * (1 - (v - lo) / (hi - lo))

                ctx.font = "12px sans-serif"
                ctx.fillStyle = "#c8c8c8"
                ctx.strokeStyle = "#3c3c46"
                ctx.lineWidth = 1

                // The horizontal grid and the labels of the vertical axis.
                for (let k = 0; k <= 4; ++k) {
                    const v = lo + ((hi - lo) * k) / 4
                    const y = yAt(v)
                    ctx.beginPath()
                    ctx.moveTo(left, y)
                    ctx.lineTo(left + w, y)
                    ctx.stroke()
                    ctx.textAlign = "right"
                    ctx.fillText(Number(v).toFixed(progressWindow.measure.digits), left - 8, y + 4)
                }

                // The labels of the horizontal axis. A long list shows some of its keys.
                const every = Math.max(1, Math.ceil(pts.length / 8))
                ctx.textAlign = "center"
                for (let i = 0; i < pts.length; i += every) {
                    ctx.fillText(pts[i].key, xAt(i), height - 12)
                }

                const color = config.themeColors.pitch
                // The range from the lowest to the highest session value of each period.
                ctx.strokeStyle = Qt.rgba(color.r, color.g, color.b, 0.45)
                ctx.lineWidth = 6
                for (let i = 0; i < pts.length; ++i) {
                    ctx.beginPath()
                    ctx.moveTo(xAt(i), yAt(pts[i].min))
                    ctx.lineTo(xAt(i), yAt(pts[i].max))
                    ctx.stroke()
                }

                // The line of the medians.
                ctx.strokeStyle = color
                ctx.lineWidth = 2
                ctx.beginPath()
                for (let i = 0; i < pts.length; ++i) {
                    if (i === 0) ctx.moveTo(xAt(i), yAt(pts[i].median))
                    else ctx.lineTo(xAt(i), yAt(pts[i].median))
                }
                ctx.stroke()
                ctx.fillStyle = color
                for (let i = 0; i < pts.length; ++i) {
                    ctx.beginPath()
                    ctx.arc(xAt(i), yAt(pts[i].median), 4, 0, 2 * Math.PI)
                    ctx.fill()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            color: "lightgrey"
            font.pixelSize: 12
            text: progressWindow.measure.name + " in " + progressWindow.measure.unit + ". The line shows the median of the sessions in each period. The bar shows the lowest and the highest session."
            wrapMode: Text.WordWrap
        }

        // The table of the same data.
        ListView {
            id: table
            Layout.fillWidth: true
            Layout.preferredHeight: 190
            clip: true
            model: progressWindow.points

            header: Rectangle {
                width: table.width
                height: 28
                color: "#2a2a33"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    Label { text: "Period"; font.bold: true; Layout.preferredWidth: 130 }
                    Label { text: "Sessions"; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                    Label { text: "Minutes"; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                    Label { text: "Median"; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                    Label { text: "Lowest"; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                    Label { text: "Highest"; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                }
            }

            delegate: Rectangle {
                required property var modelData
                required property int index
                width: table.width
                height: 26
                color: index % 2 === 0 ? "transparent" : "#1d1d24"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    Label { text: modelData.key; Layout.preferredWidth: 130 }
                    Label { text: modelData.sessions; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                    Label { text: Number(modelData.minutes).toFixed(1); Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                    Label { text: progressWindow.formatValue(modelData.median); Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                    Label { text: progressWindow.formatValue(modelData.min); Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                    Label { text: progressWindow.formatValue(modelData.max); Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }
}
