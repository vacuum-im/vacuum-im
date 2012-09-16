project(iqauth)

set(PLUGIN_NAME "iqauth")
set(PLUGIN_DISPLAY_NAME "iq:auth authentication")
set(PLUGIN_DEPENDENCIES xmppstreams) # used only in CPack

include("iqauth.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
