import QtQuick
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami

// One StatusNotifierItem, shared by the panel and the overflow popup.
// Mirrors the stock tray's StatusNotifierItem.qml.
PlasmaCore.ToolTipArea {
    id: trayIcon

    required property var model
    property QtObject backend
    property int iconSize: Kirigami.Units.iconSizes.smallMedium
    property bool highlightOnHover: false

    readonly property string source: model.DataEngineSource ?? ""

    implicitWidth: iconSize + Kirigami.Units.smallSpacing * 2
    implicitHeight: implicitWidth

    mainText: model.ToolTipTitle || model.Title || ""
    subText: model.ToolTipSubTitle || ""
    textFormat: Text.AutoText

    Rectangle {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing / 2
        radius: Kirigami.Units.cornerRadius
        visible: trayIcon.highlightOnHover && mouseArea.containsMouse
        color: Qt.alpha(Kirigami.Theme.hoverColor, 0.35)
        border.color: Qt.alpha(Kirigami.Theme.focusColor, 0.4)
    }

    Kirigami.Icon {
        anchors.centerIn: parent
        width: trayIcon.iconSize
        height: trayIcon.iconSize
        active: mouseArea.containsMouse
        source: {
            const m = trayIcon.model;
            if (m.status === PlasmaCore.Types.NeedsAttentionStatus && (m.AttentionIcon || m.AttentionIconName)) {
                return m.AttentionIcon || m.AttentionIconName;
            }
            return m.Icon || m.IconName || "";
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

        onClicked: mouse => {
            const backend = trayIcon.backend;
            if (mouse.button === Qt.RightButton || (mouse.button === Qt.LeftButton && trayIcon.model.ItemIsMenu)) {
                backend.contextMenu(trayIcon.source, mouseArea, mouse.x, mouse.y);
            } else if (mouse.button === Qt.LeftButton) {
                backend.activate(trayIcon.source, mouseArea, mouse.x, mouse.y);
            } else if (mouse.button === Qt.MiddleButton) {
                backend.secondaryActivate(trayIcon.source, mouseArea, mouse.x, mouse.y);
            }
        }

        // Some clients break on a zero delta (kmix), so only send non-zero axes
        onWheel: wheel => {
            if (wheel.angleDelta.y !== 0) {
                trayIcon.backend.scroll(trayIcon.source, wheel.angleDelta.y, "vertical");
            }
            if (wheel.angleDelta.x !== 0) {
                trayIcon.backend.scroll(trayIcon.source, wheel.angleDelta.x, "horizontal");
            }
        }
    }
}
