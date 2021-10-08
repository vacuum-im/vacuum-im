if (NOT DEFINED PLUGIN_DISPLAY_NAME)
	set(PLUGIN_DISPLAY_NAME ${PLUGIN_NAME})
endif (NOT DEFINED PLUGIN_DISPLAY_NAME)
if (NOT DEFINED PLUGIN_DESCRIPTION)
	set(PLUGIN_DESCRIPTION "${PLUGIN_DISPLAY_NAME}")
endif (NOT DEFINED PLUGIN_DESCRIPTION)

include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")
include("${CMAKE_SOURCE_DIR}/src/translations/languages.cmake") 

set(PLUGIN_${PLUGIN_NAME} YES CACHE BOOL "Enable ${PLUGIN_NAME} plugin")

set(IS_ENABLED ${PLUGIN_${PLUGIN_NAME}})

if (IS_ENABLED)
	include_directories("${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_BINARY_DIR}/src/plugins/${PLUGIN_NAME}"
		".")
	add_definitions(-DQT_PLUGIN -DQT_SHARED)

        qt6_wrap_cpp(MOC_SOURCES ${HEADERS})
        qt6_wrap_ui(UI_HEADERS ${UIS})

	if (UNIX AND NOT APPLE)
		set(CMAKE_INSTALL_RPATH "$ORIGIN/../..")
	endif (UNIX AND NOT APPLE)

	add_translations(TRANSLATIONS ${PLUGIN_NAME} ${HEADERS} ${SOURCES} ${UIS})
	add_library(${PLUGIN_NAME} SHARED ${SOURCES} ${MOC_SOURCES} ${UI_HEADERS} ${TRANSLATIONS})
        target_link_libraries(${PLUGIN_NAME} ${VACUUM_UTILS_NAME} ${ADD_LIBS} Qt6::Widgets Qt6::Xml Qt6::Network)
	if (APPLE)
		set_target_properties(${PLUGIN_NAME} PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_PLUGINS}"
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_PLUGINS}")
	else (APPLE)
		set_target_properties(${PLUGIN_NAME} PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
	endif (APPLE)
	if (WIN32)
		install(TARGETS ${PLUGIN_NAME}
			RUNTIME DESTINATION "${INSTALL_PLUGINS}"
			COMPONENT ${PLUGIN_NAME})
	elseif (NOT APPLE) # on Mac OS X they are writted to the bundle directly
		install(TARGETS ${PLUGIN_NAME}
			LIBRARY DESTINATION "${INSTALL_PLUGINS}"
			COMPONENT ${PLUGIN_NAME})
	endif (WIN32)
endif (IS_ENABLED)

if (${PLUGIN_NAME}_IS_ESSENTIAL)
	set(PLUGIN_GROUP essential_plugins)
else (${PLUGIN_NAME}_IS_ESSENTIAL)
	set(PLUGIN_GROUP optional_plugins)
endif (${PLUGIN_NAME}_IS_ESSENTIAL)

set(ALL_PLUGINS ${ALL_PLUGINS} ${PLUGIN_NAME} PARENT_SCOPE)
set(${PLUGIN_NAME}_DEPENDENCIES ${PLUGIN_DEPENDENCIES} PARENT_SCOPE)

set_property(GLOBAL APPEND PROPERTY ALL_PLUGINS_FULLPATHS "${CMAKE_INSTALL_PREFIX}/${INSTALL_PLUGINS}/${CMAKE_SHARED_LIBRARY_PREFIX}${PLUGIN_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

cpack_add_component(${PLUGIN_NAME}
	DISPLAY_NAME "${PLUGIN_DISPLAY_NAME}"
	DESCRIPTION "${PLUGIN_DESCRIPTION}"
	GROUP ${PLUGIN_GROUP}
	DEPENDS ${VACUUM_UTILS_NAME} ${PLUGIN_DEPENDENCIES})
