# needed for languages.cmake to correctly generate CPack components
set(PLUGIN_NAME ${VACUUM_LOADER_NAME})
set(PLUGIN_DISPLAY_NAME "loader")

include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}/src/loader")

include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")
include("${CMAKE_SOURCE_DIR}/src/translations/languages.cmake") 
include("loader.cmake")

project(loader CXX)

if (UNIX AND NOT APPLE)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/../${INSTALL_LIB_DIR}/")
endif (UNIX AND NOT APPLE)

set(GIT_HASH "" CACHE STRING "GIT HASH: git log -n 1 --format=%H")
set(GIT_DATE "" CACHE STRING "GIT DATE: git log -n 1 --format=%ct")

if (GIT_HASH STREQUAL "")
	execute_process(COMMAND git log -n 1 --format=%H WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_VARIABLE GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
endif (GIT_HASH STREQUAL "")

if (GIT_DATE STREQUAL "")
	execute_process(COMMAND git log -n 1 --format=%ct WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_VARIABLE GIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
endif (GIT_DATE STREQUAL "")

if (NOT (GIT_DATE STREQUAL ""))
	file(REMOVE "${CMAKE_BINARY_DIR}/src/loader/gitinfo.h")
	file(APPEND "${CMAKE_BINARY_DIR}/src/loader/gitinfo.h" "#define GIT_HASH \"${GIT_HASH}\"\n")
	file(APPEND "${CMAKE_BINARY_DIR}/src/loader/gitinfo.h" "#define GIT_DATE \"${GIT_DATE}\"\n")
	add_definitions(-DGITINFO)
endif (NOT (GIT_DATE STREQUAL ""))

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

add_translations(TRANSLATIONS "${VACUUM_LOADER_NAME}" ${SOURCES} ${UIS})
add_executable(${VACUUM_LOADER_NAME} WIN32 MACOSX_BUNDLE ${SOURCES} ${MOC_SOURCES} ${UI_HEADERS} ${TRANSLATIONS})
set_target_properties(${VACUUM_LOADER_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
target_link_libraries(${VACUUM_LOADER_NAME} ${VACUUM_UTILS_NAME} Qt5::Widgets Qt5::Xml)

install(TARGETS ${VACUUM_LOADER_NAME}
	DESTINATION "${INSTALL_BINS}"
	COMPONENT ${VACUUM_LOADER_NAME})
if (UNIX AND NOT APPLE AND NOT HAIKU)
	install(FILES "${CMAKE_SOURCE_DIR}/resources/menuicons/shared/vacuum.png"
		DESTINATION "${INSTALL_RES_DIR}/pixmaps")
	install(FILES "${CMAKE_SOURCE_DIR}/src/packages/linux/vacuum.desktop"
		DESTINATION "${INSTALL_RES_DIR}/applications")
endif (UNIX AND NOT APPLE AND NOT HAIKU)

cpack_add_component(${VACUUM_LOADER_NAME}
	DISPLAY_NAME "Loader"
	REQUIRED
	GROUP core)
