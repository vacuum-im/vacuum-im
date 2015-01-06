project(emoticons)

set(PLUGIN_NAME "emoticons")
set(PLUGIN_DISPLAY_NAME "Emoticons")
set(PLUGIN_DEPENDENCIES messagewidgets) # used only in CPack

include("emoticons.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")

if (IS_ENABLED)
	if (APPLE)
		file(COPY "${CMAKE_SOURCE_DIR}/resources/emoticons"
			DESTINATION "${CMAKE_BINARY_DIR}/${INSTALL_RESOURCES}"
			PATTERN ".svn" EXCLUDE)
	else (APPLE)
		install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/emoticons"
			DESTINATION "${INSTALL_RESOURCES}"
			COMPONENT ${PLUGIN_NAME}
			PATTERN ".svn" EXCLUDE)
	endif (APPLE)
endif (IS_ENABLED)
