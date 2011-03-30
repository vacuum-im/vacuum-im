project(optionsmanager)

set(PLUGIN_NAME "optionsmanager")
set(PLUGIN_DISPLAY_NAME "Options manager")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("optionsmanager.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	if (SYSTEM_QTLOCKEDFILE_FOUND)
		target_link_libraries(optionsmanager "${SYSTEM_QTLOCKEDFILE_FOUND}")
	else (SYSTEM_QTLOCKEDFILE_FOUND)
		target_link_libraries(optionsmanager qtlockedfile)
	endif (SYSTEM_QTLOCKEDFILE_FOUND)
endif (IS_ENABLED)
