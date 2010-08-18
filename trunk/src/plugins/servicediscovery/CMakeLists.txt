project(servicediscovery)

set(PLUGIN_NAME "servicediscovery")

include("servicediscovery.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/serviceicons"
		DESTINATION "${INSTALL_RESOURCES}"
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
