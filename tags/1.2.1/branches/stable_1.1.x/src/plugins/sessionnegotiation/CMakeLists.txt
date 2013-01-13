project(sessionnegotiation)

set(PLUGIN_NAME "sessionnegotiation")
set(PLUGIN_DISPLAY_NAME "Session negotiation")
set(PLUGIN_DEPENDENCIES dataforms stanzaprocessor) # used only in CPack

include("sessionnegotiation.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
