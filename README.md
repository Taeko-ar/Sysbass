# Compact System Tray (Sysbass)

Compact system tray plasmoid for KDE Plasma 6 (made for Bazzite). Pick which tray icons sit on the panel; the rest go into a small grid popup.

## Features

- Panel icons plus an arrow that opens a grid of the other icons. The arrow points toward the popup on any panel edge.
- Works on horizontal and vertical panels; icons stay centered in the panel's thickness.
- Each icon can be set to Panel, Overflow popup or Always hidden (Configure System Tray Icons…, saved on Apply).
- Configurable maximum columns and rows; the grid scrolls when there are more icons.
- Uses the app's theme icon or pixmap, and its attention icon when it needs attention.
- Mouse:
  - Left click activates the app. Apps that reject it (libappindicator, Electron) show their menu instead, like the stock tray.
  - Left click on Steam opens Steam's main window.
  - Middle click runs the secondary action.
  - Right click opens the app's menu (DBusMenu), with submenus, checkboxes, radio items and icons.
  - Wheel scrolls (vertical and horizontal).
- All DBus calls are async, so a hung tray app can't freeze the panel.
- On Wayland, apps get an activation token so their window comes to the front, and menus open next to the icon.

## Requirements

- KDE Plasma 6 (uses the private `org.kde.plasma.private.systemtray` QML module, tested against Plasma 6.7)
- Build: CMake ≥ 3.20, a C++17 compiler, Qt 6 (Core, Gui, Qml, Quick, DBus, Widgets), KF6 WindowSystem

The plugin must be built against the **same Qt that plasmashell uses** (the system packages). A plugin linked against Homebrew Qt loads a second Qt into plasmashell and can crash it.

## Build and install on Bazzite / Fedora Atomic

The host has no Qt dev files, so `install.sh` builds inside a Fedora distrobox whose release matches the host (Bazzite 44 = Fedora 44). Create it once:

```bash
distrobox create --name sysbass-build --image registry.fedoraproject.org/fedora:44 && distrobox enter sysbass-build -- sudo dnf install -y cmake gcc-c++ qt6-qtbase-devel qt6-qtdeclarative-devel kf6-kwindowsystem-devel dbus-daemon
```

Then build, install and restart plasmashell:

```bash
./install.sh
```

Use another container with `SYSBASS_BOX=name ./install.sh`. If the container can't see the project directory, recreate it with `--volume /var/mnt/hdd:/var/mnt/hdd`.

On a normal distro with the dev packages installed, `install.sh` builds on the host.

## Debugging

Run the widget in its own window to see QML errors without touching the panel:

```bash
plasmawindowed org.bazzite.compactsystemtray
```

Errors from the panel copy go to the journal:

```bash
journalctl --user -b -0 | grep -i compactsystemtray
```

`TypeError: Passing incompatible arguments to C++ functions` means plasmashell loaded an old plugin build. Remove any copies outside the plasmoid package (older versions installed to `~/.local/lib/qt6/qml/org/bazzite` and `~/.local/lib64/qt6/qml/org/bazzite`), then run `./install.sh` again.

## License

GPL-2.0+
