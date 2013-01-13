project(normalmessagehandler)

set(PLUGIN_NAME "normalmessagehandler")
set(PLUGIN_DISPLAY_NAME "Normal message handler")
set(PLUGIN_DEPENDENCIES messagewidgets messageprocessor messagestyles) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("normalmessagehandler.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
