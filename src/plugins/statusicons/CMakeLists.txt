project(statusicons)

set(PLUGIN_NAME "statusicons")

include("statusicons.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/statusicons"
		DESTINATION "${INSTALL_RESOURCES}"
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
