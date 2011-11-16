project(gateways)

set(PLUGIN_NAME "gateways")
set(PLUGIN_DISPLAY_NAME "Gateways")
set(PLUGIN_DEPENDENCIES stanzaprocessor) # used only in CPack

include("gateways.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
