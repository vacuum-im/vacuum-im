project(messagewidgets)

set(PLUGIN_NAME "messagewidgets")
set(PLUGIN_DISPLAY_NAME "Message widgets")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("messagewidgets.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
