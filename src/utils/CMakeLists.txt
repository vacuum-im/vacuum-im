# needed for languages.cmake to correctly generate CPack components
set(PLUGIN_NAME ${VACUUM_UTILS_NAME})
set(PLUGIN_DISPLAY_NAME "utils library")

include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")
include("${CMAKE_SOURCE_DIR}/src/translations/languages.cmake")
include("utils.cmake")

project(${VACUUM_UTILS_NAME})

add_definitions("-DUTILS_DLL")
link_directories("${CMAKE_BINARY_DIR}/libs")

set(ADD_LIBS idle)

add_translations(TRANSLATIONS "${VACUUM_UTILS_NAME}" ${SOURCES})
add_library(${VACUUM_UTILS_NAME} SHARED ${SOURCES} ${MOC_SOURCES} ${TRANSLATIONS})
set_target_properties(${VACUUM_UTILS_NAME} PROPERTIES
	VERSION "${VACUUM_UTILS_ABI}"
	SOVERSION "${VACUUM_UTILS_ABI}"
	FRAMEWORK TRUE)

if (UNIX AND NOT APPLE)
	include(FindX11)
	include_directories(AFTER ${X11_INCLUDE_DIR})
	list(APPEND ADD_LIBS ${X11_LIBRARIES})

	if (X11_Xscreensaver_FOUND)
		message(STATUS "Found Xscreensaver: ${X11_Xscreensaver_LIB}")
		list(APPEND ADD_LIBS "${X11_Xscreensaver_LIB}")
	else (X11_Xscreensaver_FOUND)
		message(STATUS "Xscreensaver not found")
	endif (X11_Xscreensaver_FOUND)
endif (UNIX AND NOT APPLE)

if (SYSTEM_IDN_FOUND)
	list(APPEND ADD_LIBS "${SYSTEM_IDN_FOUND}")
else (SYSTEM_IDN_FOUND)
	list(APPEND ADD_LIBS idn)
endif (SYSTEM_IDN_FOUND)

if (SYSTEM_ZLIB_FOUND)
	list(APPEND ADD_LIBS "${SYSTEM_ZLIB_FOUND}")
else (SYSTEM_ZLIB_FOUND)
	list(APPEND ADD_LIBS zlib)
endif (SYSTEM_ZLIB_FOUND)

if (SYSTEM_MINIZIP_FOUND)
	list(APPEND ADD_LIBS "${SYSTEM_MINIZIP_FOUND}")
else (SYSTEM_MINIZIP_FOUND)
	list(APPEND ADD_LIBS minizip)
endif (SYSTEM_MINIZIP_FOUND)

if (SYSTEM_QXTGLOBALSHORTCUT_FOUND)
	list(APPEND ADD_LIBS "${SYSTEM_QXTGLOBALSHORTCUT_FOUND}")
else (SYSTEM_QXTGLOBALSHORTCUT_FOUND)
	list(APPEND ADD_LIBS qxtglobalshortcut)
	add_definitions("-DQXT_STATIC")
endif (SYSTEM_QXTGLOBALSHORTCUT_FOUND)

if (APPLE)
	list(APPEND ADD_LIBS "-framework Carbon -framework IOKit -framework Cocoa")
endif (APPLE)

target_link_libraries(${VACUUM_UTILS_NAME} ${ADD_LIBS} Qt5::Gui Qt5::Widgets Qt5::Xml Qt5::Network)

if (WIN32)
	set_target_properties(${VACUUM_UTILS_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	install(TARGETS ${VACUUM_UTILS_NAME} RUNTIME DESTINATION "${INSTALL_LIBS}" COMPONENT ${VACUUM_UTILS_NAME})
elseif (APPLE)
	set_target_properties(${VACUUM_UTILS_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_LIBS}")
	install(TARGETS ${VACUUM_UTILS_NAME} FRAMEWORK DESTINATION "${INSTALL_LIBS}" COMPONENT ${VACUUM_UTILS_NAME})
else (WIN32)
	set_target_properties(${VACUUM_UTILS_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs")
	install(TARGETS ${VACUUM_UTILS_NAME} LIBRARY DESTINATION "${INSTALL_LIBS}" COMPONENT ${VACUUM_UTILS_NAME})
endif (WIN32)

cpack_add_component(${VACUUM_UTILS_NAME}
	DISPLAY_NAME "Utils library"
	REQUIRED
	GROUP core)

if (INSTALL_SDK)
	file(GLOB ALL_HEADERS "*.h")
	install(FILES ${ALL_HEADERS}
		DESTINATION "${INSTALL_INCLUDES}/utils"
		COMPONENT sdk)
	if (WIN32)
		install(TARGETS ${VACUUM_UTILS_NAME}
			ARCHIVE DESTINATION "${INSTALL_LIBS}"
			COMPONENT sdk)
	endif (WIN32)
endif (INSTALL_SDK)
