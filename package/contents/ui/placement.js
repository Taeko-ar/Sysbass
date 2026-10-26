// Where a tray icon goes, from the shownItems/hiddenItems/autoItems config lists.
// Shared by main.qml, configGeneral.qml and tests/tst_sysbass.cpp.
// Top-level "var" (not const) so QML can read them as Placement.Panel etc.

var Overflow = 0;
var Panel = 1;
var Hidden = 2;
var Auto = 3; // Panel while the app reports Active or NeedsAttention, else Overflow

function placement(id, shown, hidden, auto) {
    if (shown.includes(id)) {
        return Panel;
    }
    if (hidden.includes(id)) {
        return Hidden;
    }
    if (auto.includes(id)) {
        return Auto;
    }
    return Overflow;
}

// Where the icon shows right now: Panel, Overflow or Hidden
function resolve(where, active) {
    if (where === Auto) {
        return active ? Panel : Overflow;
    }
    return where;
}

// Config lists after moving id to a new placement
function setPlacement(id, where, shown, hidden, auto) {
    const lists = {
        shown: shown.filter(i => i !== id),
        hidden: hidden.filter(i => i !== id),
        auto: auto.filter(i => i !== id),
    };
    if (where === Panel) {
        lists.shown.push(id);
    } else if (where === Hidden) {
        lists.hidden.push(id);
    } else if (where === Auto) {
        lists.auto.push(id);
    }
    return lists;
}
