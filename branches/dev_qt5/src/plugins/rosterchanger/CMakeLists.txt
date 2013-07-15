project(rosterchanger)

set(PLUGIN_NAME "rosterchanger")
set(PLUGIN_DISPLAY_NAME "Roster changer")
set(PLUGIN_DEPENDENCIES roster) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("rosterchanger.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
