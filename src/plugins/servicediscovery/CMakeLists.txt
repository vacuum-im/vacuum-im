project(servicediscovery)

set(PLUGIN_NAME "servicediscovery")
set(PLUGIN_DISPLAY_NAME "Service discovery")
set(PLUGIN_DEPENDENCIES xmppstreams stanzaprocessor) # used only in CPack

include("servicediscovery.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")

if (IS_ENABLED)
	if (APPLE)
		file(COPY "${CMAKE_SOURCE_DIR}/resources/serviceicons"
			DESTINATION "${CMAKE_BINARY_DIR}/${INSTALL_RESOURCES}"
			PATTERN ".svn" EXCLUDE)
	else (APPLE)
		install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/serviceicons"
			DESTINATION "${INSTALL_RESOURCES}"
			COMPONENT ${PLUGIN_NAME}
			PATTERN ".svn" EXCLUDE)
	endif (APPLE)
endif (IS_ENABLED)
