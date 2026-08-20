import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls
import QtQml.Models
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.systemtray as SystemTray
import "org/bazzite/sysbass"

Rectangle {
    id: gridRoot

    SysbassBackend {
        id: sysbassBackend
    }

    readonly property int columnsCount: Plasmoid.configuration.maxColumns || 4
    readonly property int maxRows: Plasmoid.configuration.maxRows || 3
    readonly property int tileSize: 44
    readonly property int iconSize: 24
    readonly property int cardPadding: 12
    readonly property int itemSpacing: 6

    readonly property int itemCount: Math.max(1, count)
    readonly property int actualColumns: count === 0 ? 1 : Math.min(columnsCount, itemCount)
    readonly property int actualRows: count === 0 ? 1 : Math.min(maxRows, Math.ceil(itemCount / columnsCount))

    readonly property int calculatedWidth: count === 0 ? 180 : (actualColumns * (tileSize + itemSpacing) - itemSpacing + cardPadding * 2)
    readonly property int calculatedHeight: count === 0 ? 56 : (actualRows * (tileSize + itemSpacing) - itemSpacing + cardPadding * 2)

    implicitWidth: calculatedWidth
    implicitHeight: calculatedHeight

    Layout.preferredWidth: calculatedWidth
    Layout.preferredHeight: calculatedHeight
    Layout.minimumWidth: calculatedWidth
    Layout.minimumHeight: calculatedHeight
    Layout.maximumWidth: calculatedWidth
    Layout.maximumHeight: calculatedHeight

    color: Qt.rgba(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b, 0.92)
    radius: 12
    border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.18)
    border.width: 1

    SystemTray.StatusNotifierModel {
        id: sniModel
    }

    property int visibleCount: 0
    readonly property int count: visibleCount

    Flow {
        id: gridView
        anchors.top: parent.top
        anchors.topMargin: cardPadding
        anchors.left: parent.left
        anchors.leftMargin: cardPadding
        width: actualColumns * (tileSize + itemSpacing)
        height: actualRows * (tileSize + itemSpacing)
        spacing: itemSpacing
        clip: true
        visible: count > 0

        Repeater {
            model: sniModel
            delegate: Item {
                id: delegateItem
                
                readonly property string itemTitle: model.ToolTipTitle || model.Title || model.title || model.id || ""
                readonly property string itemId: model.Id || model.id || model.title || model.Title || "item_" + index
                readonly property string itemService: model.service || model.Service || model.id || model.Id || "item_" + index
                readonly property var itemIcon: "image://sysbass/" + encodeURIComponent(itemService)
                
                readonly property bool isVisible: {
                    var shown = Plasmoid.configuration.shownItems || [];
                    var hidden = Plasmoid.configuration.hiddenItems || [];
                    return shown.indexOf(itemId) === -1 && hidden.indexOf(itemId) === -1;
                }
                
                Component.onCompleted: {
                    if (isVisible) gridRoot.visibleCount++;
                }
                
                Component.onDestruction: {
                    if (isVisible) gridRoot.visibleCount--;
                }
                
                onIsVisibleChanged: {
                    if (isVisible) gridRoot.visibleCount++;
                    else gridRoot.visibleCount--;
                }
                
                visible: isVisible
                width: isVisible ? tileSize : 0
                height: isVisible ? tileSize : 0

                Rectangle {
                    id: tileBg
                    anchors.fill: parent
                    radius: 8
                    color: itemMouseArea.containsMouse ? Qt.rgba(Kirigami.Theme.hoverColor.r, Kirigami.Theme.hoverColor.g, Kirigami.Theme.hoverColor.b, 0.35) : "transparent"
                    border.color: itemMouseArea.containsMouse ? Qt.rgba(Kirigami.Theme.focusColor.r, Kirigami.Theme.focusColor.g, Kirigami.Theme.focusColor.b, 0.4) : "transparent"
                    border.width: itemMouseArea.containsMouse ? 1 : 0

                    Behavior on color { ColorAnimation { duration: 120 } }

                    Kirigami.Icon {
                        id: iconItem
                        anchors.centerIn: parent
                        width: iconSize
                        height: iconSize
                        source: itemIcon
                        visible: delegateItem.isVisible
                    }
                }

                MouseArea {
                    id: itemMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

                    onClicked: (mouse) => {
                        if (mouse.button === Qt.LeftButton) {
                            sysbassBackend.activate(itemService, mouse.x, mouse.y, itemMouseArea);
                        } else if (mouse.button === Qt.RightButton) {
                            sysbassBackend.contextMenu(itemService, mouse.x, mouse.y, itemMouseArea);
                        } else if (mouse.button === Qt.MiddleButton) {
                            sysbassBackend.secondaryActivate(itemService, mouse.x, mouse.y, itemMouseArea);
                        }
                    }

                    onDoubleClicked: (mouse) => {
                        if (mouse.button === Qt.LeftButton) {
                            sysbassBackend.activate(itemService, mouse.x, mouse.y, itemMouseArea);
                        }
                    }

                    onWheel: (wheel) => {
                        if (wheel.angleDelta.y !== 0) {
                            sysbassBackend.scroll(itemService, wheel.angleDelta.y, "vertical");
                        }
                    }

                    QtControls.ToolTip.visible: containsMouse && itemTitle !== ""
                    QtControls.ToolTip.text: itemTitle
                    QtControls.ToolTip.delay: 350
                }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: count === 0
        text: i18n("No hidden icons")
        color: Kirigami.Theme.disabledTextColor
        font.pixelSize: Kirigami.Theme.defaultFont.pixelSize
    }
}
