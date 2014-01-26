project(chatstates)

set(PLUGIN_NAME "chatstates")
set(PLUGIN_DISPLAY_NAME "Chat states")
set(PLUGIN_DEPENDENCIES presence messagewidgets stanzaprocessor) # used only in CPack

include("chatstates.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
