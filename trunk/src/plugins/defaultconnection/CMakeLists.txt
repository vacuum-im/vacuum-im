project(defaultconnection)

set(PLUGIN_NAME "defaultconnection")

include("defaultconnection.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (IS_ENABLED)
	target_link_libraries(defaultconnection jdns)
endif (IS_ENABLED)
