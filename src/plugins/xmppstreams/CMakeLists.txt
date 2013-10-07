project(xmppstreams)

set(PLUGIN_NAME "xmppstreams")
set(PLUGIN_DISPLAY_NAME "XMPP streams")
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("xmppstreams.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
