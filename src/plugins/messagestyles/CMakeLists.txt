project(messagestyles)

set(PLUGIN_NAME "messagestyles")
set(PLUGIN_DISPLAY_NAME "Message styles")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("messagestyles.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
