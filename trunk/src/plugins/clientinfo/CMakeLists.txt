project(clientinfo)

set(PLUGIN_NAME "clientinfo")
set(PLUGIN_DISPLAY_NAME "Client info")
set(PLUGIN_DEPENDENCIES stanzaprocessor) # used only in CPack

include("clientinfo.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
