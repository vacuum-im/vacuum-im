project(statusicons)

set(PLUGIN_NAME "statusicons")
set(PLUGIN_DISPLAY_NAME "Status icons")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("statusicons.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/statusicons"
		DESTINATION "${INSTALL_RESOURCES}"
		COMPONENT ${PLUGIN_NAME}
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
