project(optionsmanager)

set(PLUGIN_NAME "optionsmanager")

include("optionsmanager.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	target_link_libraries(optionsmanager qtlockedfile)
endif (IS_ENABLED)
