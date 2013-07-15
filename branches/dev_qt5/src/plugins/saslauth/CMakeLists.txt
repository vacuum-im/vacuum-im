project(saslauth)

set(PLUGIN_NAME "saslauth")
set(PLUGIN_DISPLAY_NAME "SASL authentication")
set(PLUGIN_DEPENDENCIES xmppstreams) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("saslauth.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
