cmake_minimum_required(VERSION 3.16)

project(sysbassplugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

list(APPEND CMAKE_PREFIX_PATH
    "/home/linuxbrew/.linuxbrew/Cellar/qtbase/6.11.1"
    "/home/linuxbrew/.linuxbrew/Cellar/qtdeclarative/6.11.1"
    "/home/linuxbrew/.linuxbrew/Cellar/qttools/6.11.1"
    "/home/linuxbrew/.linuxbrew/opt/qt"
    "/home/linuxbrew/.linuxbrew"
)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick DBus Widgets)

add_library(sysbassplugin SHARED
    src/plugin.h
    src/plugin.cpp
    src/sysbassbackend.h
    src/sysbassbackend.cpp
)

target_link_libraries(sysbassplugin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::DBus
    Qt6::Widgets
)

# Install rules to copy to local plasmoid contents
set(QML_INSTALL_DIR "$ENV{HOME}/.local/share/plasma/plasmoids/org.bazzite.compactsystemtray/contents/ui/org/bazzite/sysbass")
install(TARGETS sysbassplugin DESTINATION ${QML_INSTALL_DIR})
install(FILES qmldir DESTINATION ${QML_INSTALL_DIR})
