project(privacylists)

set(PLUGIN_NAME "privacylists")
set(PLUGIN_DISPLAY_NAME "Privacy lists")
set(PLUGIN_DEPENDENCIES xmppstreams stanzaprocessor) # used only in CPack

include("privacylists.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
