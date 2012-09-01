project(chatmessagehandler)

set(PLUGIN_NAME "chatmessagehandler")
set(PLUGIN_DISPLAY_NAME "Chat message handler")
set(PLUGIN_DEPENDENCIES messagewidgets messageprocessor messagestyles) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("chatmessagehandler.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
