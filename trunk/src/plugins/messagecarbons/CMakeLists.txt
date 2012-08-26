project(messagecarbons)

set(PLUGIN_NAME "messagecarbons")
set(PLUGIN_DISPLAY_NAME "Message Carbons")
set(PLUGIN_DEPENDENCIES xmppstreams stanzaprocessor servicediscovery) # used only in CPack

include("messagecarbons.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
