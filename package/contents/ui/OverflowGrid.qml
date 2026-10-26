import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami

// Popup content. The Plasma dialog already draws the background.
Item {
    id: gridRoot

    property alias model: repeater.model
    property QtObject backend

    readonly property int columns: Math.max(1, Plasmoid.configuration.maxColumns)
    readonly property int maxRows: Math.max(1, Plasmoid.configuration.maxRows)
    readonly property int cellSize: Kirigami.Units.gridUnit * 2 + Kirigami.Units.smallSpacing * 2

    // App icons plus the clipboard button
    readonly property int count: repeater.count + 1
    readonly property int usedColumns: Math.min(columns, count)
    readonly property int usedRows: Math.min(maxRows, Math.ceil(count / columns))
    // More items than columns x rows: the grid scrolls
    readonly property bool scrolls: count > columns * maxRows

    readonly property int contentWidth: usedColumns * cellSize + (scrolls ? scrollBar.implicitWidth : 0)
    readonly property int contentHeight: usedRows * cellSize

    Layout.minimumWidth: contentWidth
    Layout.preferredWidth: contentWidth
    Layout.maximumWidth: contentWidth
    Layout.minimumHeight: contentHeight
    Layout.preferredHeight: contentHeight
    Layout.maximumHeight: contentHeight

    Flickable {
        anchors.fill: parent
        anchors.rightMargin: gridRoot.scrolls ? scrollBar.implicitWidth : 0
        contentWidth: grid.width
        contentHeight: grid.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        // Wheel over an icon goes to the app, so scrolling uses the bar
        QQC2.ScrollBar.vertical: scrollBar

        Grid {
            id: grid
            columns: gridRoot.columns

            Repeater {
                id: repeater
                delegate: TrayIcon {
                    width: gridRoot.cellSize
                    height: gridRoot.cellSize
                    backend: gridRoot.backend
                    highlightOnHover: true
                }
            }

            // Klipper's history. The Clipboard widget must be loaded somewhere (panel or desktop).
            PlasmaCore.ToolTipArea {
                width: gridRoot.cellSize
                height: gridRoot.cellSize
                mainText: i18n("Clipboard")

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing / 2
                    radius: Kirigami.Units.cornerRadius
                    visible: clipboardMouse.containsMouse
                    color: Qt.alpha(Kirigami.Theme.hoverColor, 0.35)
                    border.color: Qt.alpha(Kirigami.Theme.focusColor, 0.4)
                }

                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: Kirigami.Units.iconSizes.smallMedium
                    height: width
                    active: clipboardMouse.containsMouse
                    source: "klipper"
                }

                MouseArea {
                    id: clipboardMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: gridRoot.backend.showClipboard()
                }
            }
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
}
