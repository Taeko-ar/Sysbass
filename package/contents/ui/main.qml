import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.systemtray as SystemTray
import "org/bazzite/sysbass"

PlasmoidItem {
    id: root

    preferredRepresentation: compactRepresentation

    // Rename built-in Plasmoid action safely if function exists
    Component.onCompleted: {
        if (typeof root.action === "function") {
            var cfgAction = root.action("configure");
            if (cfgAction) {
                cfgAction.text = i18n("Configure System Tray Icons...");
            }
        }
    }


    SysbassBackend {
        id: sysbassBackend
    }
    
    SystemTray.StatusNotifierModel {
        id: sniModel
    }

    Plasmoid.onActivated: {
        root.expanded = !root.expanded;
    }

    // Compact representation shown in panel
    compactRepresentation: Row {
        id: compactRoot
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: sniModel
            delegate: Item {
                id: mainDelegateItem
                
                readonly property string itemTitle: model.ToolTipTitle || model.Title || model.title || model.id || ""
                readonly property string itemId: model.Id || model.id || model.title || model.Title || "item_" + index
                readonly property string itemService: model.service || model.Service || model.id || model.Id || "item_" + index
                readonly property var itemIcon: "image://sysbass/" + encodeURIComponent(itemService)
                
                readonly property bool isShown: {
                    var shown = Plasmoid.configuration.shownItems || [];
                    return shown.indexOf(itemId) !== -1;
                }
                
                Component.onCompleted: {
                    console.log("[SYSBASS DEBUG] Item loaded:", itemTitle, "- ID:", model.id, "- Id:", model.Id, "- service:", model.service, "- Service:", model.Service, "- title:", model.title, "- Title:", model.Title, "- Resulting itemId:", itemId, "- itemService:", itemService);
                }
                
                visible: isShown
                width: isShown ? Kirigami.Units.iconSizes.smallMedium : 0
                height: isShown ? Kirigami.Units.iconSizes.smallMedium : 0
                
                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: Kirigami.Units.iconSizes.smallMedium
                    height: Kirigami.Units.iconSizes.smallMedium
                    source: itemIcon
                    visible: isShown
                }
                
                MouseArea {
                    id: mainMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

                    onClicked: (mouse) => {
                        if (mouse.button === Qt.LeftButton) {
                            sysbassBackend.activate(itemService, mouse.x, mouse.y, mainMouseArea);
                        } else if (mouse.button === Qt.RightButton) {
                            sysbassBackend.contextMenu(itemService, mouse.x, mouse.y, mainMouseArea);
                        } else if (mouse.button === Qt.MiddleButton) {
                            sysbassBackend.secondaryActivate(itemService, mouse.x, mouse.y, mainMouseArea);
                        }
                    }

                    onDoubleClicked: (mouse) => {
                        if (mouse.button === Qt.LeftButton) {
                            sysbassBackend.activate(itemService, mouse.x, mouse.y, mainMouseArea);
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

        Item {
            implicitWidth: Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.smallSpacing * 2
            implicitHeight: Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.smallSpacing * 2

            PlasmaCore.ToolTipArea {
                anchors.fill: parent
                active: false
            }

            Rectangle {
                id: compactBg
                anchors.fill: parent
                radius: 4
                color: mouseArea.containsMouse ? Kirigami.Theme.hoverColor : "transparent"
                border.color: mouseArea.containsMouse ? Kirigami.Theme.focusColor : "transparent"
                border.width: 1

                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: Kirigami.Units.iconSizes.smallMedium
                    height: Kirigami.Units.iconSizes.smallMedium
                    source: "pan-up-symbolic"
                    isMask: true
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton

                onClicked: (mouse) => {
                    if (mouse.button === Qt.LeftButton) {
                        root.expanded = !root.expanded;
                    }
                }
            }
        }
    }

    // Expanded representation shown in overflow popup window
    fullRepresentation: OverflowGrid {
        id: overflowGrid
    }
}
