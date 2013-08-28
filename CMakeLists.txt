cmake_minimum_required(VERSION 2.8)

project(vacuum-im)

find_package (Qt5Widgets REQUIRED)
find_package (Qt5LinguistTools REQUIRED)

add_custom_target(updatets)

include("src/make/config.cmake")
include("src/translations/languages.cmake")
include_directories("${CMAKE_SOURCE_DIR}/src")

install(DIRECTORY "resources/menuicons"
	DESTINATION "${INSTALL_RESOURCES}"
	PATTERN ".svn" EXCLUDE)

include("src/packages/package.cmake")

set(INSTALL_SDK YES CACHE BOOL "Install files needed to build custom plugins")
set(INSTALL_DOCS YES CACHE BOOL "Install documentation (currently it's only AUTHORS, CHANGELOG, README and COPYING)")
set(LANGS "" CACHE STRING "List of languages to build localization for (this variable overrides any of LANG_*)")
set(USED_LANGS "" CACHE INTERNAL "List of languages actually used for generating targets")
add_lang_options()

set(FORCE_BUNDLED_MINIZIP NO CACHE BOOL "Use bundled minizip even if local system have one")

find_library(SYSTEM_ZLIB_FOUND z)
find_library(SYSTEM_IDN_FOUND idn)
find_library(SYSTEM_MINIZIP_FOUND minizip)
find_library(SYSTEM_QTLOCKEDFILE_FOUND QtSolutions_LockedFile-2.4)

if (FORCE_BUNDLED_MINIZIP)
	set(SYSTEM_MINIZIP_FOUND NO)
endif (FORCE_BUNDLED_MINIZIP)

if (SYSTEM_ZLIB_FOUND)
	add_definitions(-DUSE_SYSTEM_ZLIB)
endif (SYSTEM_ZLIB_FOUND)

if (SYSTEM_IDN_FOUND)
	add_definitions(-DUSE_SYSTEM_IDN)
endif (SYSTEM_IDN_FOUND)

if (SYSTEM_MINIZIP_FOUND)
	add_definitions(-DUSE_SYSTEM_MINIZIP)
endif (SYSTEM_MINIZIP_FOUND)

if (SYSTEM_QTLOCKEDFILE_FOUND)
	add_definitions(-DUSE_SYSTEM_QTLOCKEDFILE)
endif (SYSTEM_QTLOCKEDFILE_FOUND)

if ("${LANGS}" STREQUAL "")
	process_lang_options()
else ("${LANGS}" STREQUAL "")
	set_enabled_langs("${LANGS}")
endif ("${LANGS}" STREQUAL "")

if (INSTALL_DOCS)
	install(FILES "AUTHORS" "CHANGELOG" "README" "COPYING" "TRANSLATORS"
		DESTINATION "${INSTALL_DOCUMENTS}")
endif (INSTALL_DOCS)

add_subdirectory(src)

# runtime deps on Windows
if (WIN32)
	cpack_add_component_group(libs
		DISPLAY_NAME "Shared libraries"
		DESCRIPTION "Required libraries that already may be present in your system")
	set(QT_LIBS "${_qt5Core_install_prefix}/bin/icudt51.dll"
		"${_qt5Core_install_prefix}/bin/icuin51.dll"
		"${_qt5Core_install_prefix}/bin/icuuc51.dll"
		"${_qt5Core_install_prefix}/bin/libEGL.dll"
		"${_qt5Core_install_prefix}/bin/libGLESv2.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Core.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Gui.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Multimedia.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Network.dll"
		"${_qt5Core_install_prefix}/bin/Qt5OpenGL.dll"
		"${_qt5Core_install_prefix}/bin/Qt5PrintSupport.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Qml.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Quick.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Sensors.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Sql.dll"
		"${_qt5Core_install_prefix}/bin/Qt5V8.dll"
		"${_qt5Core_install_prefix}/bin/Qt5WebKit.dll"
		"${_qt5Core_install_prefix}/bin/Qt5WebKitWidgets.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Widgets.dll"
		"${_qt5Core_install_prefix}/bin/Qt5Xml.dll"
		"${_qt5Core_install_prefix}/bin/Qt5XmlPatterns.dll")
	install(FILES ${QT_LIBS}
		DESTINATION "${INSTALL_LIBS}"
		COMPONENT qt_libs)
	install(DIRECTORY "${_qt5Core_install_prefix}/plugins/imageformats"
		DESTINATION "${INSTALL_LIBS}"
		COMPONENT qt_libs
		FILES_MATCHING
		PATTERN "*.dll"
		PATTERN "*d.dll" EXCLUDE)
	install(DIRECTORY "${_qt5Core_install_prefix}/plugins/platforms"
		DESTINATION "${INSTALL_LIBS}"
		COMPONENT qt_libs
		FILES_MATCHING
		PATTERN "qwindows.dll")
	install(DIRECTORY "${_qt5Core_install_prefix}/plugins/sensors"
		DESTINATION "${INSTALL_LIBS}"
		COMPONENT qt_libs
		FILES_MATCHING
		PATTERN "*.dll"
		PATTERN "*d.dll" EXCLUDE)
	cpack_add_component(qt_libs
		DISPLAY_NAME "Qt5 libraries"
		DESCRIPTION "May be unchecked if you already have working Qt5 installation"
		GROUP libs)
	# Qt localization
	foreach(LANG ${LOCALIZED_LANGS})
		string(SUBSTRING ${LANG} 0 2 SHORTLANG)
		install(FILES "${_qt5Core_install_prefix}/translations/qt_${SHORTLANG}.qm"
			DESTINATION "${INSTALL_TRANSLATIONS}/${LANG}"
			COMPONENT qt_${LANG})
		cpack_add_component(qt_${LANG}
			DISPLAY_NAME "Qt5 translation"
			GROUP ${LANG}_translation
			DEPENDS qt_libs)
	endforeach(LANG)

#	install(FILES "libeay32.dll" "ssleay32.dll"
#		DESTINATION "${INSTALL_LIBS}"
#		COMPONENT openssl_libs)
#	cpack_add_component(openssl_libs
#		DISPLAY_NAME "OpenSSL libraries"
#		DESCRIPTION "May be unchecked if you already have OpenSSL installed"
#		GROUP libs)

	if (MINGW)
		install(FILES "mingwm10.dll" "libgcc_s_dw2-1.dll"
			DESTINATION "${INSTALL_LIBS}"
			COMPONENT mingw_runtime)
		cpack_add_component(mingw_runtime
			DISPLAY_NAME "MinGW runtime libraries"
			DESCRIPTION "May be unchecked if you already have MinGW installed"
			GROUP libs)
	elseif (MSVC)
		# TODO: check these patterns, indeed, don't work.
		file(GLOB MSVC_DLLS "msvc*.dll")
		file(GLOB MANIFEST "Microsoft.VC*.CRT.manifest")
		install(FILES ${MSVC_DLLS} ${MANIFEST}
			DESTINATION "${INSTALL_LIBS}"
			COMPONENT msvc_runtime)
		cpack_add_component(msvc_runtime
			DISPLAY_NAME "MSVC runtime libraries"
			DESCRIPTION "May be unchecked if you already have MS Visual C++ or it's redistributable package installed"
			GROUP libs)
	endif (MINGW)
endif (WIN32)

if (INSTALL_SDK)
	cpack_add_component(sdk
		DISPLAY_NAME "SDK"
		DESCRIPTION "Files needed to build custom plugins"
		DISABLED)
endif (INSTALL_SDK)
