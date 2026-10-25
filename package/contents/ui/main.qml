import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels
import org.kde.plasma.private.systemtray as SystemTray
import "org/bazzite/sysbass"

PlasmoidItem {
    id: root

    preferredRepresentation: compactRepresentation

    // Each icon has its own tooltip; an applet-wide one would stack on top of it
    toolTipMainText: ""
    toolTipSubText: ""

    Component.onCompleted: Plasmoid.internalAction("configure").text = i18n("Configure System Tray Icons…")

    SysbassBackend {
        id: sysbassBackend
    }

    SystemTray.StatusNotifierModel {
        id: sniModel
        readonly property int idRole: KItemModels.KRoleNames.role("Id")

        function itemId(row, parent) {
            return sniModel.data(sniModel.index(row, 0, parent), idRole) ?? "";
        }
    }

    // Rebinding the callback (it reads the config) makes the proxy refilter on config changes
    KItemModels.KSortFilterProxyModel {
        id: panelModel
        sourceModel: sniModel
        filterRowCallback: {
            const shown = Plasmoid.configuration.shownItems;
            return (row, parent) => shown.includes(sniModel.itemId(row, parent));
        }
    }

    KItemModels.KSortFilterProxyModel {
        id: overflowModel
        sourceModel: sniModel
        filterRowCallback: {
            const shown = Plasmoid.configuration.shownItems;
            const hidden = Plasmoid.configuration.hiddenItems;
            return (row, parent) => {
                const id = sniModel.itemId(row, parent);
                return !shown.includes(id) && !hidden.includes(id);
            };
        }
    }

    compactRepresentation: GridLayout {
        id: compact
        readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical

        flow: vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        rowSpacing: 0
        columnSpacing: 0

        Layout.minimumWidth: vertical ? 0 : implicitWidth
        Layout.preferredWidth: Layout.minimumWidth
        Layout.minimumHeight: vertical ? implicitHeight : 0
        Layout.preferredHeight: Layout.minimumHeight

        Repeater {
            model: panelModel
            delegate: TrayIcon {
                backend: sysbassBackend
                // Fill the panel's thickness; the icon centers itself inside
                Layout.fillHeight: !compact.vertical
                Layout.fillWidth: compact.vertical
            }
        }

        PlasmaCore.ToolTipArea {
            id: expander

            implicitWidth: Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.smallSpacing * 2
            implicitHeight: implicitWidth
            Layout.fillHeight: !compact.vertical
            Layout.fillWidth: compact.vertical
            mainText: root.expanded ? i18n("Hide icons") : i18n("Show hidden icons")

            Rectangle {
                anchors.fill: parent
                radius: Kirigami.Units.cornerRadius
                color: expanderMouse.containsMouse ? Kirigami.Theme.hoverColor : "transparent"
                border.color: expanderMouse.containsMouse ? Kirigami.Theme.focusColor : "transparent"
            }

            Kirigami.Icon {
                anchors.centerIn: parent
                width: Kirigami.Units.iconSizes.smallMedium
                height: width
                isMask: true
                // Arrow points away from the panel edge, toward where the popup opens
                source: {
                    switch (Plasmoid.location) {
                    case PlasmaCore.Types.TopEdge: return "pan-down-symbolic";
                    case PlasmaCore.Types.LeftEdge: return "pan-end-symbolic";
                    case PlasmaCore.Types.RightEdge: return "pan-start-symbolic";
                    default: return "pan-up-symbolic";
                    }
                }
            }

            MouseArea {
                id: expanderMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.expanded = !root.expanded
            }
        }
    }

    fullRepresentation: OverflowGrid {
        model: overflowModel
        backend: sysbassBackend
    }
}
