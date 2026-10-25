#!/usr/bin/env bash
# Builds the C++ plugin and installs/upgrades the plasmoid for the current user.
# Build runs on the host when Qt6 dev files exist, else inside distrobox $SYSBASS_BOX.
set -euo pipefail

PACKAGE_ID="org.bazzite.compactsystemtray"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$SCRIPT_DIR/package"
PLUGIN_DIR="$PACKAGE_DIR/contents/ui/org/bazzite/sysbass"
BUILD_DIR="$SCRIPT_DIR/build"
BOX="${SYSBASS_BOX:-sysbass-build}"

build="rm -rf '$BUILD_DIR' && cmake -S '$SCRIPT_DIR' -B '$BUILD_DIR' -DCMAKE_BUILD_TYPE=Release && cmake --build '$BUILD_DIR' -j\$(nproc) && ctest --test-dir '$BUILD_DIR' --output-on-failure"

echo "=== Building plugin ==="
if [ -e /usr/lib64/cmake/Qt6Qml/Qt6QmlConfig.cmake ] || [ -e /usr/lib/cmake/Qt6Qml/Qt6QmlConfig.cmake ]; then
    bash -c "$build"
else
    echo "No Qt6 dev files on host, building in distrobox '$BOX'"
    # Qt refuses a plugin built against a newer major.minor than it runs
    host_qt=$(rpm -q --qf '%{VERSION}' qt6-qtbase | cut -d. -f1,2)
    box_qt=$(distrobox enter "$BOX" -- rpm -q --qf '%{VERSION}' qt6-qtbase | tr -d '\r' | cut -d. -f1,2)
    if [ "$(printf '%s\n%s\n' "$host_qt" "$box_qt" | sort -V | tail -1)" != "$host_qt" ]; then
        echo "Container Qt $box_qt is newer than host Qt $host_qt; plasmashell would refuse the plugin" >&2
        exit 1
    fi
    distrobox enter "$BOX" -- bash -c "$build"
fi

# Plasma loads the plugin straight from the package directory
mkdir -p "$PLUGIN_DIR"
rm -f "$PLUGIN_DIR/sysbassplugin.so"
cp -f "$BUILD_DIR/libsysbassplugin.so" "$PLUGIN_DIR/"

echo "=== Installing plasmoid $PACKAGE_ID ==="
if kpackagetool6 --type Plasma/Applet --show "$PACKAGE_ID" >/dev/null 2>&1; then
    kpackagetool6 --type Plasma/Applet --upgrade "$PACKAGE_DIR"
else
    kpackagetool6 --type Plasma/Applet --install "$PACKAGE_DIR"
fi

# A loaded plugin can't be swapped, so plasmashell has to restart
if systemctl --user is-active --quiet plasma-plasmashell.service; then
    echo "Restarting plasmashell..."
    systemctl --user restart plasma-plasmashell.service
fi
