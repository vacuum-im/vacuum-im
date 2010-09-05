project(messagearchiver)

set(PLUGIN_NAME "messagearchiver")
set(PLUGIN_DISPLAY_NAME "Message archiver")
set(PLUGIN_DEPENDENCIES xmppstreams stanzaprocessor) # used only in CPack

include("messagearchiver.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
