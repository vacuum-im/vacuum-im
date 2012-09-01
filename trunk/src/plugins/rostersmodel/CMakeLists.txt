project(rostersmodel)

set(PLUGIN_NAME "rostersmodel")
set(PLUGIN_DISPLAY_NAME "Rosters model")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("rostersmodel.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
