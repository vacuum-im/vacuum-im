project(recentcontacts)

set(PLUGIN_NAME "recentcontacts")
set(PLUGIN_DISPLAY_NAME "Recent Contacts")
set(PLUGIN_DEPENDENCIES privatestorage) # used only in CPack

include("recentcontacts.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
