project(compress)

set(PLUGIN_NAME "compress")
set(PLUGIN_DISPLAY_NAME "Stream compression")
set(PLUGIN_DEPENDENCIES xmppstreams) # used only in CPack

include_directories(AFTER "${CMAKE_SOURCE_DIR}/src/thirdparty/zlib")

include("compress.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
