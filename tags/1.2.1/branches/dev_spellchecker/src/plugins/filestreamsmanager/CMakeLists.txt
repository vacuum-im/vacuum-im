project(filestreamsmanager)

set(PLUGIN_NAME "filestreamsmanager")
set(PLUGIN_DISPLAY_NAME "File streams manager")
set(PLUGIN_DEPENDENCIES datastreamsmanager) # used only in CPack

include("filestreamsmanager.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
