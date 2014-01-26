project(adiummessagestyle)

set(PLUGIN_NAME "adiummessagestyle")
set(PLUGIN_DISPLAY_NAME "Adium message style")
set(QT_USE_QTWEBKIT YES)
set(QT_USE_PHONON NO)
set(QT_USE_QTXMLPATTERNS NO)

include("adiummessagestyle.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/adiummessagestyles"
		DESTINATION "${INSTALL_RESOURCES}"
		COMPONENT ${PLUGIN_NAME}
		PATTERN ".svn" EXCLUDE)
endif (IS_ENABLED)
