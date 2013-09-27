project(statistics)

set(PLUGIN_NAME "statistics")
set(PLUGIN_DISPLAY_NAME "Statistics")

include("statistics.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")

if (IS_ENABLED)
	qt5_use_modules(${PLUGIN_NAME} WebKit WebKitWidgets)
endif (IS_ENABLED)
