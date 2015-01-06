project(simplemessagestyle)

set(PLUGIN_NAME "simplemessagestyle")
set(PLUGIN_DISPLAY_NAME "Simple message style")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("simplemessagestyle.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")

if (IS_ENABLED)
	if (APPLE)
		file(COPY "${CMAKE_SOURCE_DIR}/resources/simplemessagestyles"
			DESTINATION "${CMAKE_BINARY_DIR}/${INSTALL_RESOURCES}"
			PATTERN ".svn" EXCLUDE)
	else (APPLE)
		install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/simplemessagestyles"
			DESTINATION "${INSTALL_RESOURCES}"
			COMPONENT ${PLUGIN_NAME}
			PATTERN ".svn" EXCLUDE)
	endif (APPLE)
endif (IS_ENABLED)
