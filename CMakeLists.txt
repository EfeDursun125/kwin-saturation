# SPDX-FileCopyrightText: 2026 EfeDursun125
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)
project(kwin-saturation-effect VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_MAJOR_VERSION 6)
set(QT_DEFAULT_MAJOR_VERSION 6)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(Qt6 6.0 REQUIRED COMPONENTS Core Gui Widgets DBus Svg Network)
find_package(KF6Config REQUIRED)

# KWin development headers/CMake config and transitive dependencies.
# On Arch/CachyOS, KWin's Vulkan dependency check requires vulkan-headers.
find_package(KWin REQUIRED)

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

add_executable(kwin-saturation-gui
	gui_main.cpp
	resources.qrc
)

target_link_libraries(kwin-saturation-gui
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::DBus
		Qt6::Svg
		Qt6::Network
		KF6::ConfigCore
)

# Install executable
install(TARGETS kwin-saturation-gui
	DESTINATION ${KDE_INSTALL_BINDIR}
)

install(PROGRAMS kwin-saturation
	DESTINATION ${KDE_INSTALL_BINDIR}
)

# Install desktop file to system applications menu
install(FILES kwin-saturation-gui.desktop
	DESTINATION ${KDE_INSTALL_APPDIR}
)

# Install icon across standard theme resolutions and pixmaps
set(ICON_SIZES scalable 256x256 128x128 64x64 48x48 32x32)
foreach(SIZE ${ICON_SIZES})
	install(FILES kwin-saturation.svg
		DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/${SIZE}/apps
		RENAME kwin-saturation.svg
	)
endforeach()

install(FILES kwin-saturation.svg
	DESTINATION ${KDE_INSTALL_DATADIR}/pixmaps
	RENAME kwin-saturation.svg
)

# Copy launcher to User's Desktop as "Digital Vibrance" (without .desktop) upon installation
set(PROJ_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
install(CODE "
	set(SOURCE_DIR \"${PROJ_SOURCE_DIR}\")
	if(DEFINED ENV{SUDO_USER} AND NOT \"\$ENV{SUDO_USER}\" STREQUAL \"root\")
		set(TARGET_DESKTOP_DIR \"/home/\$ENV{SUDO_USER}/Desktop\")
		set(USER_NAME \"\$ENV{SUDO_USER}\")
	else()
		set(TARGET_DESKTOP_DIR \"\$ENV{HOME}/Desktop\")
		set(USER_NAME \"\")
	endif()

	if(EXISTS \"\${TARGET_DESKTOP_DIR}\")
		message(STATUS \"Creating Desktop launcher in \${TARGET_DESKTOP_DIR}...\")
		file(REMOVE
			\"\${TARGET_DESKTOP_DIR}/kwin-saturation-gui.desktop\"
			\"\${TARGET_DESKTOP_DIR}/KWin Saturation.desktop\"
			\"\${TARGET_DESKTOP_DIR}/Digital Vibrance.desktop\"
			\"\${TARGET_DESKTOP_DIR}/Digital Vibrance\"
		)
		file(COPY \"\${SOURCE_DIR}/kwin-saturation-gui.desktop\" DESTINATION \"\${TARGET_DESKTOP_DIR}\")
		file(RENAME \"\${TARGET_DESKTOP_DIR}/kwin-saturation-gui.desktop\" \"\${TARGET_DESKTOP_DIR}/Digital Vibrance\")
		execute_process(COMMAND chmod +x \"\${TARGET_DESKTOP_DIR}/Digital Vibrance\")
		execute_process(COMMAND gio set \"\${TARGET_DESKTOP_DIR}/Digital Vibrance\" metadata::trusted true ERROR_QUIET OUTPUT_QUIET)
		if(NOT \"\${USER_NAME}\" STREQUAL \"\" AND DEFINED ENV{SUDO_UID} AND DEFINED ENV{SUDO_GID})
			execute_process(COMMAND chown \"\$ENV{SUDO_UID}:\$ENV{SUDO_GID}\" \"\${TARGET_DESKTOP_DIR}/Digital Vibrance\")
		endif()
	endif()

	if(NOT \"\${USER_NAME}\" STREQUAL \"\")
		execute_process(COMMAND rm -rf \"/home/\${USER_NAME}/.cache/thumbnails\" \"/home/\${USER_NAME}/.cache/icon-cache.kcache\" ERROR_QUIET OUTPUT_QUIET)
	endif()

	execute_process(COMMAND touch /usr/share/icons/hicolor ERROR_QUIET OUTPUT_QUIET)
	execute_process(COMMAND gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor ERROR_QUIET OUTPUT_QUIET)
	execute_process(COMMAND kbuildsycoca6 --noincremental ERROR_QUIET OUTPUT_QUIET)
")
