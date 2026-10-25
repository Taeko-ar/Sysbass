cmake_minimum_required(VERSION 3.16)

project(sysbassplugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Build against the same Qt/KF6 that plasmashell runs on (system packages), never Homebrew's
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick DBus Widgets Test)
find_package(KF6WindowSystem REQUIRED)

add_library(sysbassplugin MODULE
    src/plugin.cpp
    src/sysbassbackend.cpp
)

target_link_libraries(sysbassplugin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::DBus
    Qt6::Widgets
    KF6::WindowSystem
)

enable_testing()
add_executable(tst_sysbass tests/tst_sysbass.cpp src/sysbassbackend.cpp)
target_link_libraries(tst_sysbass PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::DBus Qt6::Widgets Qt6::Test KF6::WindowSystem)
target_compile_definitions(tst_sysbass PRIVATE PLACEMENT_JS="${CMAKE_CURRENT_SOURCE_DIR}/package/contents/ui/placement.js")
# Private session bus with a fake tray app; no display needed
add_test(NAME tst_sysbass COMMAND dbus-run-session -- $<TARGET_FILE:tst_sysbass>)
set_tests_properties(tst_sysbass PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
