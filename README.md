# Compact System Tray (Sysbass)

Compact system tray plasmoid widget for KDE Plasma 6 (mostly for Bazzite) to group and manage system tray applications.

## Features

- **Compact Grid & Overflow Menu:** Organize system tray icons into a clean compact layout with an expandable overflow grid.
- **Customizable Layout:** Configure maximum rows and columns.
- **Item Visibility Controls:** Choose which application icons appear on the main panel or stay inside the overflow popup.
- **Native StatusNotifierItem Integration:** Native C++ backend plugin bridging Qt/QML and DBus StatusNotifierItem interfaces.

## Requirements

- KDE Plasma 6
- Qt 6 (Core, Gui, Qml, Quick, DBus, Widgets)
- CMake (>= 3.16)
- C++17 compiler (`g++` or `clang++`)

## Installation & Build

Run the installation script to compile the C++ plugin, register the plasmoid package, and refresh `plasmashell`:

```bash
chmod +x ./install.sh
./install.sh
```

### Manual Build

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

## License

GPL-2.0+
