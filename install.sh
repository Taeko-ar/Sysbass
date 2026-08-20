#!/usr/bin/env bash
set -e

PACKAGE_ID="org.bazzite.compactsystemtray"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$SCRIPT_DIR/package"

echo "=== Building Sysbass C++ Plugin ==="
rm -rf "$SCRIPT_DIR/build"
mkdir -p "$SCRIPT_DIR/build"
cd "$SCRIPT_DIR/build"
cmake "$SCRIPT_DIR" -DCMAKE_PREFIX_PATH="/home/linuxbrew/.linuxbrew/opt/qt;/home/linuxbrew/.linuxbrew"
make -j$(nproc)

TARGET_QML_DIR="$PACKAGE_DIR/contents/ui/org/bazzite/sysbass"
mkdir -p "$TARGET_QML_DIR"
cp -f "$SCRIPT_DIR/qmldir" "$TARGET_QML_DIR/"
cp -f "$SCRIPT_DIR/build/libsysbassplugin.so" "$TARGET_QML_DIR/"
cp -f "$SCRIPT_DIR/build/libsysbassplugin.so" "$TARGET_QML_DIR/sysbassplugin.so"

# Also copy to global user Qt6 import paths for system-wide QML loader lookup
GLOBAL_QT_DIR="$HOME/.local/lib/qt6/qml/org/bazzite/sysbass"
mkdir -p "$GLOBAL_QT_DIR"
cp -f "$SCRIPT_DIR/qmldir" "$GLOBAL_QT_DIR/"
cp -f "$SCRIPT_DIR/build/libsysbassplugin.so" "$GLOBAL_QT_DIR/"
cp -f "$SCRIPT_DIR/build/libsysbassplugin.so" "$GLOBAL_QT_DIR/sysbassplugin.so"

GLOBAL_QT64_DIR="$HOME/.local/lib64/qt6/qml/org/bazzite/sysbass"
mkdir -p "$GLOBAL_QT64_DIR"
cp -f "$SCRIPT_DIR/qmldir" "$GLOBAL_QT64_DIR/"
cp -f "$SCRIPT_DIR/build/libsysbassplugin.so" "$GLOBAL_QT64_DIR/"
cp -f "$SCRIPT_DIR/build/libsysbassplugin.so" "$GLOBAL_QT64_DIR/sysbassplugin.so"

echo "=== Installing/Updating Plasmoid Package $PACKAGE_ID ==="

if kpackagetool6 --type Plasma/Applet --list | grep -q "$PACKAGE_ID"; then
    echo "Upgrading existing plasmoid package $PACKAGE_ID..."
    kpackagetool6 --type Plasma/Applet --upgrade "$PACKAGE_DIR"
else
    echo "Installing new plasmoid package $PACKAGE_ID..."
    kpackagetool6 --type Plasma/Applet --install "$PACKAGE_DIR"
fi

echo "=== Plasmoid package updated in ~/.local/share/plasma/plasmoids ==="

if systemctl --user is-active --quiet plasma-plasmashell.service 2>/dev/null; then
    echo "Restarting plasma-plasmashell systemd service..."
    systemctl --user restart plasma-plasmashell.service
    echo "plasmashell restarted safely via systemd!"
fi
