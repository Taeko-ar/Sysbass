import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.systemtray as SystemTray

Kirigami.FormLayout {
    id: configPage
    anchors.fill: parent

    SystemTray.StatusNotifierModel {
        id: sniModel
    }

    property alias cfg_maxColumns: maxColumnsSpin.value
    property alias cfg_maxRows: maxRowsSpin.value

    Item {
        Kirigami.FormData.isSection: true
        Kirigami.FormData.label: i18n("Layout")
    }

    PlasmaComponents.SpinBox {
        id: maxColumnsSpin
        Kirigami.FormData.label: i18n("Maximum columns:")
        from: 1
        to: 16
    }

    PlasmaComponents.SpinBox {
        id: maxRowsSpin
        Kirigami.FormData.label: i18n("Maximum rows:")
        from: 1
        to: 16
    }

    Item {
        Kirigami.FormData.isSection: true
        Kirigami.FormData.label: i18n("App Icons")
    }

    Text {
        text: i18n("Configure which icons appear in the overflow grid vs main panel:")
        color: Kirigami.Theme.disabledTextColor
        font.pixelSize: Kirigami.Theme.defaultFont.pixelSize
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    ListView {
        id: itemsList
        Layout.fillWidth: true
        implicitHeight: Math.max(200, contentHeight)
        clip: true
        model: sniModel

        delegate: Rectangle {
            width: itemsList.width
            height: 40
            radius: 6
            color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.05)

            readonly property string itemId: model.Id || model.id || model.title || model.Title || "item_" + index
            readonly property string itemService: model.service || model.Service || model.id || model.Id || "item_" + index
            readonly property string itemTitle: model.Title || model.title || model.ToolTipTitle || itemId
            readonly property var itemIcon: "image://sysbass/" + encodeURIComponent(itemService)

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 10

                Kirigami.Icon {
                    source: itemIcon
                    width: 24
                    height: 24
                }

                Text {
                    text: itemTitle
                    color: Kirigami.Theme.textColor
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                QtControls.ComboBox {
                    model: [i18n("Overflow Grid"), i18n("Main Panel"), i18n("Always Hidden")]
                    currentIndex: {
                        var shown = Plasmoid.configuration.shownItems || [];
                        var hidden = Plasmoid.configuration.hiddenItems || [];
                        if (shown.indexOf(itemId) !== -1) return 1;
                        if (hidden.indexOf(itemId) !== -1) return 2;
                        return 0;
                    }
                    onActivated: (index) => {
                        var shown = Plasmoid.configuration.shownItems || [];
                        var hidden = Plasmoid.configuration.hiddenItems || [];
                        var extra = Plasmoid.configuration.extraItems || [];
                        
                        shown = shown.filter(i => i !== itemId);
                        hidden = hidden.filter(i => i !== itemId);
                        extra = extra.filter(i => i !== itemId);
                        
                        if (index === 1) shown.push(itemId);
                        else if (index === 2) hidden.push(itemId);
                        else extra.push(itemId);
                        
                        Plasmoid.configuration.shownItems = shown;
                        Plasmoid.configuration.hiddenItems = hidden;
                        Plasmoid.configuration.extraItems = extra;
                    }
                    implicitWidth: 140
                }
            }
        }
    }
}
