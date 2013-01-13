project(defaultconnection)

set(PLUGIN_NAME "defaultconnection")
set(PLUGIN_DISPLAY_NAME "Default connection")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("defaultconnection.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	target_link_libraries(defaultconnection jdns)
endif (IS_ENABLED)
