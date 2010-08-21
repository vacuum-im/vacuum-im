project(commands)

set(PLUGIN_NAME "commands")
set(PLUGIN_DISPLAY_NAME "Ad-hoc commands")
set(PLUGIN_DEPENDENCIES dataforms xmppstreams stanzaprocessor) # used only in CPack

include("commands.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
