# SPDX-FileCopyrightText: 2026 EfeDursun125
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)
project(kwin-saturation-effect VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(KF6Config REQUIRED)

# kwin development headers/CMake config
find_package(KWin REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)

add_library(saturationeffect MODULE
    saturationeffect.cpp
    main.cpp
)

target_link_libraries(saturationeffect
    PRIVATE
        Qt6::Core
        Qt6::Gui
        KF6::ConfigCore
        KWin::kwin
)

# make metadata.json available to moc (Q_PLUGIN_METADATA ... FILE "metadata.json")
# so it gets embedded into the plugin binary
configure_file(metadata.json ${CMAKE_CURRENT_BINARY_DIR}/metadata.json COPYONLY)

install(TARGETS saturationeffect
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins
)
