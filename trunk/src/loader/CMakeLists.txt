# needed for translations.cmake to correctly generate CPack components
set(PLUGIN_NAME ${TARGET_LOADER})
set(PLUGIN_DISPLAY_NAME "loader")

include("${CMAKE_SOURCE_DIR}/config.cmake")
include("loader.cmake")

project(loader CXX)

include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}/src/loader")
set(QT_USE_QTXML YES)
include(${QT_USE_FILE})

set(VER_STRING "" CACHE STRING "This string will be appended to program version (defaults to SVN revision number)")

if (VER_STRING STREQUAL "")
execute_process(COMMAND svnversion -n -c "${CMAKE_SOURCE_DIR}"
	OUTPUT_VARIABLE SVN_REVISION)
else (VER_STRING STREQUAL "")
	set(SVN_REVISION "0:${VER_STRING}") # see PluginManager::revision()
endif (VER_STRING STREQUAL "")

if (NOT (SVN_REVISION STREQUAL ""))
	file(REMOVE "svninfo.h")
	file(WRITE "svninfo.h" "#define SVN_REVISION \"${SVN_REVISION}\"\n")
	add_definitions(-DSVNINFO)
endif (NOT (SVN_REVISION STREQUAL ""))

if (WIN32)
	if (MINGW)
		add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/loader.rc.obj"
			COMMAND windres.exe
				"-I${CMAKE_CURRENT_SOURCE_DIR}"
				"-i${CMAKE_CURRENT_SOURCE_DIR}/loader.rc"
				-o "${CMAKE_CURRENT_BINARY_DIR}/loader.rc.obj")
		set(SOURCES ${SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/loader.rc.obj")
	else (MINGW)
		set(SOURCES ${SOURCES} "loader.rc")
	endif (MINGW)
endif (WIN32)

add_translations(TRANSLATIONS "${TARGET_LOADER}" ${SOURCES} ${UIS})
add_executable(${TARGET_LOADER} WIN32 ${SOURCES} ${MOC_SOURCES} ${UI_HEADERS} ${TRANSLATIONS})
set_target_properties(${TARGET_LOADER} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
target_link_libraries(${TARGET_LOADER} ${TARGET_UTILS} ${QT_LIBRARIES})

install(TARGETS ${TARGET_LOADER}
	RUNTIME	DESTINATION "${INSTALL_BINS}"
	COMPONENT ${TARGET_LOADER})
if (UNIX AND NOT APPLE)
	install(FILES "${CMAKE_SOURCE_DIR}/resources/menuicons/shared/vacuum.png"
		DESTINATION "${INSTALL_RES_DIR}/pixmaps")
	install(FILES "${CMAKE_SOURCE_DIR}/src/packages/linux/vacuum.desktop"
		DESTINATION "${INSTALL_RES_DIR}/applications")
endif (UNIX AND NOT APPLE)

cpack_add_component(${TARGET_LOADER}
	DISPLAY_NAME "Loader"
	REQUIRED
	GROUP core)
