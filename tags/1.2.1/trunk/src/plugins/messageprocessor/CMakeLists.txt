project(messageprocessor)

set(PLUGIN_NAME "messageprocessor")
set(PLUGIN_DISPLAY_NAME "Message processor")
set(PLUGIN_DEPENDENCIES xmppstreams stanzaprocessor) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("messageprocessor.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
