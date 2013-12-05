project(adiummessagestyle)

set(PLUGIN_NAME "adiummessagestyle")
set(PLUGIN_DISPLAY_NAME "Adium message style")

include("adiummessagestyle.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")

if (IS_ENABLED)
	qt5_use_modules(${PLUGIN_NAME} WebKit WebKitWidgets)
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/adiummessagestyles"
		DESTINATION "${INSTALL_RESOURCES}"
		COMPONENT ${PLUGIN_NAME}
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
