project(adiummessagestyle)

set(PLUGIN_NAME "adiummessagestyle")
set(QT_USE_QTWEBKIT YES)

include("adiummessagestyle.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/adiummessagestyles"
		DESTINATION "${INSTALL_RESOURCES}"
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
