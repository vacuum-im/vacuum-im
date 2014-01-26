project(connectionmanager)

set(PLUGIN_NAME "connectionmanager")
set(PLUGIN_DISPLAY_NAME "Connection manager")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("connectionmanager.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
