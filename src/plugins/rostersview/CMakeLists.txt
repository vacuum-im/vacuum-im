project(rostersview)

set(PLUGIN_NAME "rostersview")
set(PLUGIN_DISPLAY_NAME "Rosters view")
set(PLUGIN_DEPENDENCIES rostersmodel) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("rostersview.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
