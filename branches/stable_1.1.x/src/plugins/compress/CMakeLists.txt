project(compress)

set(PLUGIN_NAME "compress")
set(PLUGIN_DISPLAY_NAME "Stream compression")
set(PLUGIN_DEPENDENCIES xmppstreams) # used only in CPack

if (SYSTEM_ZLIB_FOUND)
	set(ADD_LIBS ${SYSTEM_ZLIB_FOUND})
else (SYSTEM_ZLIB_FOUND)
	include_directories(AFTER "${CMAKE_SOURCE_DIR}/src/thirdparty/zlib")
	set(ADD_LIBS zlib)
endif (SYSTEM_ZLIB_FOUND)

include("compress.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
