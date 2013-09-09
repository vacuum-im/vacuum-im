project(statuschanger)

set(PLUGIN_NAME "statuschanger")
set(PLUGIN_DISPLAY_NAME "Status changer")
set(PLUGIN_DEPENDENCIES presence) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("statuschanger.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
