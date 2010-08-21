project(remotecontrol)

set(PLUGIN_NAME "remotecontrol")
set(PLUGIN_DISPLAY_NAME "Remote control")
set(PLUGIN_DEPENDENCIES commands dataforms) # used only in CPack

include("remotecontrol.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
