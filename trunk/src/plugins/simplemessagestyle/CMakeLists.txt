project(simplemessagestyle)

set(PLUGIN_NAME "simplemessagestyle")

include("simplemessagestyle.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/simplemessagestyles"
		DESTINATION "${INSTALL_RESOURCES}"
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
