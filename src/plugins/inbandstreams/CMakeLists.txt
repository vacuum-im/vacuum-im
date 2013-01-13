project(inbandstreams)

set(PLUGIN_NAME "inbandstreams")
set(PLUGIN_DISPLAY_NAME "In-band streams")
set(PLUGIN_DEPENDENCIES stanzaprocessor) # used only in CPack

include("inbandstreams.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
