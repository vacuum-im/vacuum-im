project(roster)

set(PLUGIN_NAME "roster")
set(PLUGIN_DISPLAY_NAME "Roster")
set(PLUGIN_DEPENDENCIES xmppstreams stanzaprocessor) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("roster.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
