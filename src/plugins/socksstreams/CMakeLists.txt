project(socksstreams)

set(PLUGIN_NAME "socksstreams")
set(PLUGIN_DISPLAY_NAME "SOCKS streams")
set(PLUGIN_DEPENDENCIES stanzaprocessor) # used only in CPack

include("socksstreams.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
