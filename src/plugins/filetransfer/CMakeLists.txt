project(filetransfer)

set(PLUGIN_NAME "filetransfer")
set(PLUGIN_DISPLAY_NAME "File transfer")
set(PLUGIN_DEPENDENCIES filestreamsmanager datastreamsmanager) # used only in CPack

include("filetransfer.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
