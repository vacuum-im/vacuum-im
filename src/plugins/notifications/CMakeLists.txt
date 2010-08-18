project(notifications)

set(PLUGIN_NAME "notifications")

include("notifications.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/sounds"
		DESTINATION "${INSTALL_RESOURCES}"
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
