project(notifications)

set(PLUGIN_NAME "notifications")
set(PLUGIN_DISPLAY_NAME "Notifications")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("notifications.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")

if (IS_ENABLED)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/sounds"
		DESTINATION "${INSTALL_RESOURCES}"
		COMPONENT ${PLUGIN_NAME}
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
