# needed for translations.cmake to correctly generate CPack components
set(PLUGIN_NAME ${TARGET_UTILS})
set(PLUGIN_DISPLAY_NAME "utils library")

include("${CMAKE_SOURCE_DIR}/config.cmake")
include("utils.cmake")

project(${TARGET_UTILS})

set(QT_USE_QTXML YES)
include(${QT_USE_FILE})

add_definitions("-DUTILS_DLL")
add_definitions("-DQXT_STATIC") 
link_directories("${CMAKE_BINARY_DIR}/libs")
include_directories(AFTER "${CMAKE_SOURCE_DIR}/src/thirdparty/zlib")

add_translations(TRANSLATIONS "${TARGET_UTILS}" ${SOURCES})
add_library(${TARGET_UTILS} SHARED ${SOURCES} ${MOC_SOURCES} ${TRANSLATIONS})
set_target_properties(${TARGET_UTILS} PROPERTIES
	VERSION "${VERSION_UTILS}"
	SOVERSION "${VERSION_UTILS_ABI}")

if (UNIX AND NOT APPLE)
	set(ADD_LIBS qxtglobalshortcut idn minizip "-lz")
else (UNIX AND NOT APPLE)
	set(ADD_LIBS qxtglobalshortcut idn minizip zlib)
endif (UNIX AND NOT APPLE)

target_link_libraries(${TARGET_UTILS} LINK_INTERFACE_LIBRARIES ${QT_LIBRARIES})
target_link_libraries(${TARGET_UTILS} ${QT_LIBRARIES} ${ADD_LIBS})

if (WIN32)
	set_target_properties(${TARGET_UTILS} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	install(TARGETS ${TARGET_UTILS}
		RUNTIME DESTINATION "${INSTALL_LIBS}"
		COMPONENT ${TARGET_UTILS})
else (WIN32)
	set_target_properties(${TARGET_UTILS} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs")
	install(TARGETS ${TARGET_UTILS}
		LIBRARY DESTINATION "${INSTALL_LIBS}"
		COMPONENT ${TARGET_UTILS})
endif (WIN32)

cpack_add_component(${TARGET_UTILS}
	DISPLAY_NAME "Utils library"
	REQUIRED
	GROUP core)

if (INSTALL_SDK)
	file(GLOB ALL_HEADERS "*.h")
	install(FILES ${ALL_HEADERS}
		DESTINATION "${INSTALL_INCLUDES}/utils"
		COMPONENT sdk)
	if (WIN32)
		install(TARGETS ${TARGET_UTILS}
			ARCHIVE DESTINATION "${INSTALL_LIBS}"
			COMPONENT sdk)
	endif (WIN32)
endif (INSTALL_SDK)
