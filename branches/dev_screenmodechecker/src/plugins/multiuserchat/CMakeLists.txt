project(multiuserchat)

set(PLUGIN_NAME "multiuserchat")
set(PLUGIN_DISPLAY_NAME "Multi-user chat")
set(PLUGIN_DEPENDENCIES stanzaprocessor) # used only in CPack

include("multiuserchat.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
