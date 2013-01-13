project(servermessagearchive)

set(PLUGIN_NAME "servermessagearchive")
set(PLUGIN_DISPLAY_NAME "Server Message Archive")
set(PLUGIN_DEPENDENCIES messagearchiver stanzaprocessor) # used only in CPack

include("servermessagearchive.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
