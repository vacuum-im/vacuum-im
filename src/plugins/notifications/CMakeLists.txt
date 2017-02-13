project(notifications)

set(PLUGIN_NAME "notifications")
set(PLUGIN_DISPLAY_NAME "Notifications")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("notifications.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")

if (IS_ENABLED)
	find_package(Qt5Multimedia REQUIRED)
	target_link_libraries(${PLUGIN_NAME} Qt5::Multimedia)
	if (APPLE)
		file(COPY "${CMAKE_SOURCE_DIR}/resources/sounds"
			DESTINATION "${CMAKE_BINARY_DIR}/${INSTALL_RESOURCES}")
	else (APPLE)
		install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/sounds"
			DESTINATION "${INSTALL_RESOURCES}"
			COMPONENT ${PLUGIN_NAME})
	endif (APPLE)
endif (IS_ENABLED)
