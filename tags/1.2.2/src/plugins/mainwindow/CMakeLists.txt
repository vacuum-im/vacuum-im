project(mainwindow)

set(PLUGIN_NAME "mainwindow")
set(PLUGIN_DISPLAY_NAME "Main window")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("mainwindow.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
