if (WIN32)
	set(CPACK_GENERATOR NSIS)
	#set(CPACK_PACKAGE_EXECUTABLES ${VACUUM_LOADER_NAME} "Vacuum-IM")
	set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Vacuum-IM.lnk' '\$INSTDIR\\\\vacuum.exe'")
	set(CPACK_NSIS_CONTACT "http://www.vacuum-im.org")
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "VacuumIM")
endif (WIN32)

if (APPLE)
	set(CPACK_GENERATOR "DragNDrop")
	set(CPACK_DMG_FORMAT "UDBZ")
	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/src/packages/macosx/background.png")
	set(CPACK_DMG_VOLUME_NAME "Vacuum-IM")
	set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/src/packages/macosx/DS_Store")
	set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/vacuum.icns")
endif (APPLE)

execute_process(COMMAND svnversion -n "${CMAKE_SOURCE_DIR}"
	OUTPUT_VARIABLE SVNREVISION)

set(CPACK_PACKAGE_NAME "Vacuum-IM")
set(VER_MAJOR 1)
set(VER_MINOR 3)
set(VER_PATCH 0)
set(VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}")

if (NOT (SVNREVISION STREQUAL "" OR SVNREVISION STREQUAL "Unversioned directory"))
	set(VERSION "${VERSION}-r${SVNREVISION}")
endif (NOT (SVNREVISION STREQUAL "" OR SVNREVISION STREQUAL "Unversioned directory"))

set(CPACK_PACKAGE_VERSION_MAJOR ${VER_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VER_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VER_PATCH})
set(CPACK_PACKAGE_VENDOR "http://vacuum-im.googlecode.com")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Vacuum-IM")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")
if (NOT APPLE)
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
endif (NOT APPLE)
set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_SOURCE_DIR}/CHANGELOG")
set(CPACK_PACKAGE_FILE_NAME "vacuum-im-${VERSION}-installer")

include(CPack)

cpack_add_component_group(core
	DISPLAY_NAME "Core components"
	DESCRIPTION "Loader and utils library")
cpack_add_component_group(essential_plugins
	DISPLAY_NAME "Essential plugins"
	DESCRIPTION "Minimal set of plugins required for basic messaging and presence functionality")
cpack_add_component_group(optional_plugins
	DISPLAY_NAME "Optional plugins"
	DESCRIPTION "Plugins that not required, but still useful")
cpack_add_component_group(translations
	DISPLAY_NAME "Translations"
	DESCRIPTION "Translations to various languages")
foreach(LANG ${LOCALIZED_LANGS})
	lang_display_name(LANG_NAME ${LANG})
	cpack_add_component_group(${LANG}_translation
		DISPLAY_NAME "${LANG_NAME} translation"
		DESCRIPTION "${LANG_NAME} translation"
		PARENT_GROUP translations)
endforeach(LANG)
