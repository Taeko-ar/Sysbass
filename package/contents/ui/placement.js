// Where a tray icon goes, from the shownItems/hiddenItems config lists.
// Shared by main.qml, configGeneral.qml and tests/tst_sysbass.cpp.
// Top-level "var" (not const) so QML can read them as Placement.Panel etc.

var Overflow = 0;
var Panel = 1;
var Hidden = 2;

function placement(id, shown, hidden) {
    if (shown.includes(id)) {
        return Panel;
    }
    if (hidden.includes(id)) {
        return Hidden;
    }
    return Overflow;
}

// Config lists after moving id to a new placement
function setPlacement(id, where, shown, hidden) {
    const newShown = shown.filter(i => i !== id);
    const newHidden = hidden.filter(i => i !== id);
    if (where === Panel) {
        newShown.push(id);
    } else if (where === Hidden) {
        newHidden.push(id);
    }
    return { shown: newShown, hidden: newHidden };
}
