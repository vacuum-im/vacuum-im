project(filemessagearchive)

set(PLUGIN_NAME "filemessagearchive")
set(PLUGIN_DISPLAY_NAME "File Message Archive")
set(PLUGIN_DEPENDENCIES messagearchiver) # used only in CPack

include("filemessagearchive.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
