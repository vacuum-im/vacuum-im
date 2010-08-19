project(compress)

set(PLUGIN_NAME "compress")

include_directories(AFTER "${CMAKE_SOURCE_DIR}/src/thirdparty/zlib")

include("compress.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
