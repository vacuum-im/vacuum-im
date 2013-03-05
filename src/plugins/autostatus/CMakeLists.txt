project(autostatus)

set(PLUGIN_NAME "autostatus")
set(PLUGIN_DISPLAY_NAME "Autostatus")
set(PLUGIN_DEPENDENCIES statuschanger accountmanager) # used only in CPack

include("autostatus.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
