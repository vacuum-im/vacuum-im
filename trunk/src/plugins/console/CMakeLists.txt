project(console)

set(PLUGIN_NAME "console")
set(PLUGIN_DISPLAY_NAME "XML console")
set(PLUGIN_DEPENDENCIES xmppstreams mainwindow) # used only in CPack

include("console.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
