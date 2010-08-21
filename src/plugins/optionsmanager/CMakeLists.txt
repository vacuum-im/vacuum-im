project(optionsmanager)

set(PLUGIN_NAME "optionsmanager")
set(PLUGIN_DISPLAY_NAME "Options manager")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("optionsmanager.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	target_link_libraries(optionsmanager qtlockedfile)
endif (IS_ENABLED)
