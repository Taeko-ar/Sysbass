cmake_minimum_required(VERSION 3.16)

project(sysbassplugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Build against the same Qt/KF6 that plasmashell runs on (system packages), never Homebrew's
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick DBus Widgets)
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
add_executable(sysbass_selftest tests/selftest.cpp src/sysbassbackend.cpp)
target_link_libraries(sysbass_selftest PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::DBus Qt6::Widgets KF6::WindowSystem)
add_test(NAME selftest COMMAND sysbass_selftest)
