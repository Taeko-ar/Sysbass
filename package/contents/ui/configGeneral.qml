import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.plasma.private.systemtray as SystemTray

KCM.SimpleKCM {
    id: configPage

    // Plasma binds cfg_* to config entries and saves them on Apply
    property alias cfg_maxColumns: maxColumnsSpin.value
    property alias cfg_maxRows: maxRowsSpin.value
    property var cfg_shownItems: []
    property var cfg_hiddenItems: []

    readonly property QtObject sniModel: SystemTray.StatusNotifierModel {}

    Kirigami.FormLayout {
        QQC2.SpinBox {
            id: maxColumnsSpin
            Kirigami.FormData.label: i18n("Maximum columns:")
            from: 1
            to: 16
        }

        QQC2.SpinBox {
            id: maxRowsSpin
            Kirigami.FormData.label: i18n("Maximum rows:")
            from: 1
            to: 16
        }

        Item {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Icons")
        }

        QQC2.Label {
            text: i18n("Only running apps are listed.")
            opacity: 0.6
        }

        Repeater {
            model: configPage.sniModel

            delegate: RowLayout {
                id: row

                required property var model
                readonly property string itemId: model.Id ?? ""

                Kirigami.FormData.label: model.Title || model.ToolTipTitle || itemId

                Kirigami.Icon {
                    source: row.model.Icon || row.model.IconName || ""
                    implicitWidth: Kirigami.Units.iconSizes.smallMedium
                    implicitHeight: implicitWidth
                }

                QQC2.ComboBox {
                    model: [i18n("Overflow popup"), i18n("Panel"), i18n("Always hidden")]
                    currentIndex: configPage.cfg_shownItems.includes(row.itemId) ? 1
                                : configPage.cfg_hiddenItems.includes(row.itemId) ? 2 : 0
                    onActivated: index => {
                        const shown = configPage.cfg_shownItems.filter(id => id !== row.itemId);
                        const hidden = configPage.cfg_hiddenItems.filter(id => id !== row.itemId);
                        if (index === 1) {
                            shown.push(row.itemId);
                        } else if (index === 2) {
                            hidden.push(row.itemId);
                        }
                        configPage.cfg_shownItems = shown;
                        configPage.cfg_hiddenItems = hidden;
                    }
                }
            }
        }
    }
}
