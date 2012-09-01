project(stanzaprocessor)

set(PLUGIN_NAME "stanzaprocessor")
set(PLUGIN_DISPLAY_NAME "Stanza processor")
set(PLUGIN_DEPENDENCIES xmppstreams) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("stanzaprocessor.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
