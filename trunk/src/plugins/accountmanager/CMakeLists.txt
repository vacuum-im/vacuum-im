project(accountmanager)

set(PLUGIN_NAME "accountmanager")
set(PLUGIN_DISPLAY_NAME "Account manager")
set(PLUGIN_DEPENDENCIES xmppstreams) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("accountmanager.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
