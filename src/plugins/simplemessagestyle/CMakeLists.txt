project(simplemessagestyle)

set(PLUGIN_NAME "simplemessagestyle")
set(PLUGIN_DISPLAY_NAME "Simple message style")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("simplemessagestyle.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/simplemessagestyles"
		DESTINATION "${INSTALL_RESOURCES}"
		COMPONENT ${PLUGIN_NAME}
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
