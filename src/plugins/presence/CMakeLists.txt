project(presence)

set(PLUGIN_NAME "presence")
set(PLUGIN_DISPLAY_NAME "Presence")
set(PLUGIN_DEPENDENCIES xmppstreams stanzaprocessor) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("presence.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
