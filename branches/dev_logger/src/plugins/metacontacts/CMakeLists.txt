project(metacontacts)

set(PLUGIN_NAME "metacontacts")
set(PLUGIN_DISPLAY_NAME "Meta-Contacts")
set(PLUGIN_DEPENDENCIES privatestorage) # used only in CPack

include("metacontacts.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
