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
    height: 880
    minimumWidth: 580
    minimumHeight: 560

    font.pixelSize: ui.type.body
    Material.theme: Material.Dark
    // One corner radius for buttons, fields, lists, and popups. It is the medium radius of the design scale.
    Material.roundedScale: Material.SmallScale
    Material.accent: Material.DeepPurple

    // The measure is a track. The statistic is one value of that track in a session.
    readonly property var measures: [
        { name: "Pitch", key: "pitch" },
        { name: "F1", key: "f1" },
        { name: "F2", key: "f2" },
        { name: "F3", key: "f3" },
        { name: "F4", key: "f4" }
    ]
    readonly property var stats: [
        { name: "Median", key: "median", unit: "Hz", digits: 1 },
        { name: "Mean", key: "mean", unit: "Hz", digits: 1 },
        { name: "Standard deviation (SD)", key: "sd", unit: "Hz", digits: 1 },
        { name: "Variation (SD / mean)", key: "cv", unit: "%", digits: 1 },
        { name: "Minimum", key: "min", unit: "Hz", digits: 1 },
        { name: "5th percentile (P5)", key: "p5", unit: "Hz", digits: 1 },
        { name: "25th percentile (P25)", key: "p25", unit: "Hz", digits: 1 },
        { name: "75th percentile (P75)", key: "p75", unit: "Hz", digits: 1 },
        { name: "95th percentile (P95)", key: "p95", unit: "Hz", digits: 1 },
        { name: "Maximum", key: "max", unit: "Hz", digits: 1 },
        { name: "Range (P95 minus P5), in Hz", key: "range", unit: "Hz", digits: 1 },
        { name: "Range (P95 to P5), in semitones", key: "range_st", unit: "semitones", digits: 1 },
        { name: "Voiced share", key: "voiced", unit: "%", digits: 0 },
        { name: "Number of values", key: "count", unit: "values", digits: 0 }
    ]
    readonly property var periods: [
        { name: "Day", key: "day" },
        { name: "Week", key: "week" },
        { name: "Month", key: "month" },
        { name: "Year", key: "year" }
    ]

    property var points: []
    property var sessionList: []
    property var tagNames: ["All tasks"]
    readonly property var measure: measures[measureBox.currentIndex]
    readonly property var stat: stats[statBox.currentIndex]

    function refresh() {
        const names = practice.libraryTags()
        const keep = taskBox.currentText
        tagNames = ["All tasks"].concat(names)
        // The task of the last practice is the default filter, because a formant over mixed tasks says little.
        const wanted = keep && tagNames.indexOf(keep) >= 0 ? keep : (tagNames.indexOf(practice.tag) >= 0 ? practice.tag : "All tasks")
        taskBox.currentIndex = Math.max(0, tagNames.indexOf(wanted))
        points = practice.trend(measure.key, stat.key, periods[periodBox.currentIndex].key, taskBox.currentIndex > 0 ? tagNames[taskBox.currentIndex] : "")
        sessionList = practice.sessions()
        chart.requestPaint()
    }

    function formatValue(v) {
        return Number(v).toFixed(stat.digits)
    }

    onVisibleChanged: if (visible) refresh()

    Connections {
        target: practice
        function onSaved() { if (progressWindow.visible) progressWindow.refresh() }
        function onDeleted() { if (progressWindow.visible) progressWindow.refresh() }
        // A new folder of the sessions has other sessions.
        function onFolderChanged() { if (progressWindow.visible) progressWindow.refresh() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ui.space.lg
        spacing: ui.space.md

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Each box has a small title above it.
            ColumnLayout {
                Layout.preferredWidth: 110
                spacing: 2
                Label { text: "Measure"; color: ui.color.secondary; font.pixelSize: ui.type.caption }
                ComboBox {
                    id: measureBox
                    Layout.fillWidth: true
                    model: progressWindow.measures.map(m => m.name)
                    onActivated: progressWindow.refresh()
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label { text: "Statistic"; color: ui.color.secondary; font.pixelSize: ui.type.caption }
                ComboBox {
                    id: statBox
                    Layout.fillWidth: true
                    model: progressWindow.stats.map(m => m.name)
                    onActivated: progressWindow.refresh()
                }
            }
            ColumnLayout {
                Layout.preferredWidth: 170
                spacing: 2
                Label { text: "Task"; color: ui.color.secondary; font.pixelSize: ui.type.caption }
                ComboBox {
                    id: taskBox
                    implicitContentWidthPolicy: ComboBox.WidestText
                    Layout.fillWidth: true
                    model: progressWindow.tagNames
                    onActivated: progressWindow.refresh()
                }
            }
            ColumnLayout {
                Layout.preferredWidth: 110
                spacing: 2
                Label { text: "Period"; color: ui.color.secondary; font.pixelSize: ui.type.caption }
                ComboBox {
                    id: periodBox
                    Layout.fillWidth: true
                    model: progressWindow.periods.map(p => p.name)
                    currentIndex: 1
                    onActivated: progressWindow.refresh()
                }
            }
        }

        Hint {
            text: "Measure is the track. Statistic is the value of that track in each session. Task keeps the sessions of one task. Period groups the sessions by day, week, month, or year."
        }

        Label {
            Layout.fillWidth: true
            Layout.maximumWidth: ui.measure
            visible: progressWindow.points.length === 0
            wrapMode: Text.WordWrap
            color: ui.color.secondary
            text: "No session has a value for this choice. Practice with the Practice button, or choose another task, measure, or statistic."
        }

        Canvas {
            id: chart
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            Layout.verticalStretchFactor: 3
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

                ctx.font = "12px '" + ui.font.mono + "'"
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
                    ctx.fillText(Number(v).toFixed(progressWindow.stat.digits), left - 8, y + 4)
                }

                // The labels of the horizontal axis. A long list shows some of its keys.
                const every = Math.max(1, Math.ceil(pts.length / 8))
                ctx.textAlign = "center"
                for (let i = 0; i < pts.length; i += every) {
                    ctx.fillText(pts[i].key, xAt(i), height - 12)
                }

                const colors = config.themeColors
                const color = progressWindow.measure.key === "pitch" ? colors.pitch : colors.formants[Math.max(0, Number(progressWindow.measure.key.substring(1)) - 1)]
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
            Layout.maximumWidth: ui.measure
            color: ui.color.secondary
            font.pixelSize: ui.type.caption
            text: progressWindow.stat.name + " of " + progressWindow.measure.name + ", in " + progressWindow.stat.unit + ". The line shows the median of the sessions in each period. The bar shows the lowest and the highest session."
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
                    Label { text: modelData.key; Layout.preferredWidth: 130; font.family: ui.font.mono }
                    Label { text: modelData.sessions; font.family: ui.font.mono; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                    Label { text: Number(modelData.minutes).toFixed(1); font.family: ui.font.mono; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                    Label { text: progressWindow.formatValue(modelData.median); font.family: ui.font.mono; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                    Label { text: progressWindow.formatValue(modelData.min); font.family: ui.font.mono; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                    Label { text: progressWindow.formatValue(modelData.max); font.family: ui.font.mono; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                }
            }

            ScrollBar.vertical: ScrollBar {}
            NativeWheel { flickable: table }
        }

        // The sessions themselves, newest first, each with a button to delete it.
        Label {
            Layout.topMargin: ui.space.sm
            text: "Sessions"
            font.pixelSize: ui.type.title
        }

        Label {
            Layout.fillWidth: true
            Layout.maximumWidth: ui.measure
            visible: progressWindow.sessionList.length === 0
            wrapMode: Text.WordWrap
            color: ui.color.secondary
            text: "No practice session is saved yet."
        }

        ListView {
            id: sessionTable
            objectName: "sessionTable"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 128
            Layout.verticalStretchFactor: 2
            visible: progressWindow.sessionList.length > 0
            clip: true
            model: progressWindow.sessionList

            header: Rectangle {
                width: sessionTable.width
                height: 32
                color: "#2a2a33"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: ui.space.sm
                    anchors.rightMargin: ui.space.sm
                    Label { text: "Date"; font.bold: true; Layout.preferredWidth: 150 }
                    Label { text: "Task"; font.bold: true; Layout.preferredWidth: 110 }
                    Label { text: "Minutes"; font.bold: true; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                    Label { text: "Pitch (Hz)"; font.bold: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                    Label { text: "Marks"; font.bold: true; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight }
                    Item { Layout.preferredWidth: 88 }
                }
            }

            delegate: Rectangle {
                required property var modelData
                required property int index
                width: sessionTable.width
                height: 32
                color: index % 2 === 0 ? "transparent" : "#1d1d24"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: ui.space.sm
                    anchors.rightMargin: ui.space.sm
                    Label { text: modelData.start; Layout.preferredWidth: 150; font.family: ui.font.mono }
                    Label { text: modelData.tag === "" ? "-" : modelData.tag; Layout.preferredWidth: 110; elide: Text.ElideRight }
                    Label { text: Number(modelData.minutes).toFixed(1); font.family: ui.font.mono; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                    Label { text: modelData.pitch > 0 ? Number(modelData.pitch).toFixed(1) : "-"; font.family: ui.font.mono; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                    Label { text: modelData.marks; font.family: ui.font.mono; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight }
                    Button {
                        objectName: "deleteSession"
                        text: "Delete"
                        flat: true
                        Layout.preferredWidth: 88
                        Layout.preferredHeight: 32
                        // A new session is not in the list yet, so a practice in progress is never at risk.
                        onClicked: {
                            deleteDialog.target = modelData
                            deleteDialog.open()
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}
            NativeWheel { flickable: sessionTable }
        }
    }

    // The question before a delete. The sound, the statistics, the marks, and the report of the session are lost.
    Dialog {
        id: deleteDialog
        objectName: "deleteSessionDialog"
        Material.roundedScale: Material.SmallScale
        title: "Delete this practice session?"
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel
        property var target: null

        Component.onCompleted: standardButton(Dialog.Ok).text = "Delete"
        onAccepted: {
            if (target) practice.deleteSession(target.folder)
            target = null
        }
        onRejected: target = null

        ColumnLayout {
            spacing: ui.space.md
            Label {
                Layout.preferredWidth: 420
                wrapMode: Text.WordWrap
                text: deleteDialog.target
                    ? deleteDialog.target.start + ", " + (deleteDialog.target.tag === "" ? "no task" : deleteDialog.target.tag)
                      + ", " + Number(deleteDialog.target.minutes).toFixed(1) + " minutes"
                    : ""
            }
            Label {
                Layout.preferredWidth: 420
                wrapMode: Text.WordWrap
                text: "This deletes the statistics, the marks, the report, and the sound of this session from the folder of the practice sessions. Files that you exported before stay. You cannot undo this."
            }
        }
    }
}
