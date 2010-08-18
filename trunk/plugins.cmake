include("${CMAKE_SOURCE_DIR}/config.cmake")

set(${PLUGIN_NAME}_ENABLE YES CACHE BOOL "Enable ${PLUGIN_NAME} plugin")

set(IS_ENABLED ${${PLUGIN_NAME}_ENABLE})

if (IS_ENABLED)
	set(QT_USE_QTNETWORK YES)
	set(QT_USE_QTXML YES)
	include(${QT_USE_FILE})

	link_directories("${CMAKE_BINARY_DIR}/src/libs")
	include_directories("${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_BINARY_DIR}/src/plugins/${PLUGIN_NAME}"
		".")
	add_definitions(-DQT_PLUGIN -DQT_SHARED)

	qt4_wrap_cpp(MOC_SOURCES ${HEADERS})
	qt4_wrap_ui(UI_HEADERS ${UIS})

	add_translations(TRANSLATIONS ${PLUGIN_NAME} ${HEADERS} ${SOURCES} ${UIS})
	add_library(${PLUGIN_NAME} SHARED ${SOURCES} ${MOC_SOURCES} ${UI_HEADERS} ${TRANSLATIONS})
	set_target_properties(${PLUGIN_NAME} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
	target_link_libraries(${PLUGIN_NAME} ${TARGET_UTILS} ${QT_LIBRARIES} ${ADD_LIBS})
	install(TARGETS ${PLUGIN_NAME}
		LIBRARY DESTINATION "${INSTALL_PLUGINS}")
endif (IS_ENABLED)
