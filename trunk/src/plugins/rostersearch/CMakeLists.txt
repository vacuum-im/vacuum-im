project(rostersearch)

set(PLUGIN_NAME "rostersearch")
set(PLUGIN_DISPLAY_NAME "Roster search")
set(PLUGIN_DEPENDENCIES rostersview mainwindow) # used only in CPack

include("rostersearch.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
