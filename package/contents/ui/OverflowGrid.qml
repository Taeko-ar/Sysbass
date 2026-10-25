import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami

// Popup content. The Plasma dialog already draws the background.
Item {
    id: gridRoot

    property alias model: grid.model
    property QtObject backend

    readonly property int columns: Math.max(1, Plasmoid.configuration.maxColumns)
    readonly property int maxRows: Math.max(1, Plasmoid.configuration.maxRows)
    readonly property int cellSize: Kirigami.Units.gridUnit * 2 + Kirigami.Units.smallSpacing * 2

    readonly property int count: grid.count
    readonly property int usedColumns: Math.max(1, Math.min(columns, count))
    readonly property int usedRows: Math.max(1, Math.min(maxRows, Math.ceil(count / columns)))
    // More items than columns x rows: the grid scrolls
    readonly property bool scrolls: count > columns * maxRows

    readonly property int contentWidth: count === 0
        ? emptyLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
        : usedColumns * cellSize + (scrolls ? scrollBar.implicitWidth : 0)
    readonly property int contentHeight: count === 0 ? cellSize : usedRows * cellSize

    Layout.minimumWidth: contentWidth
    Layout.preferredWidth: contentWidth
    Layout.maximumWidth: contentWidth
    Layout.minimumHeight: contentHeight
    Layout.preferredHeight: contentHeight
    Layout.maximumHeight: contentHeight

    GridView {
        id: grid
        anchors.fill: parent
        anchors.rightMargin: gridRoot.scrolls ? scrollBar.implicitWidth : 0
        cellWidth: gridRoot.cellSize
        cellHeight: gridRoot.cellSize
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        // Wheel over an icon goes to the app, so scrolling uses the bar
        QQC2.ScrollBar.vertical: scrollBar

        delegate: TrayIcon {
            width: gridRoot.cellSize
            height: gridRoot.cellSize
            backend: gridRoot.backend
            highlightOnHover: true
        }
    }

    // Sibling of the grid, so it can sit in the right margin next to it
    PlasmaComponents.ScrollBar {
        id: scrollBar
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: gridRoot.scrolls
    }

    PlasmaComponents.Label {
        id: emptyLabel
        anchors.centerIn: parent
        visible: gridRoot.count === 0
        text: i18n("No hidden icons")
        opacity: 0.6
    }
}
