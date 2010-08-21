project(starttls)

set(PLUGIN_NAME "starttls")
set(PLUGIN_DISPLAY_NAME "StartTLS")
set(PLUGIN_DEPENDENCIES xmppstreams defaultconnection) # used only in CPack
set(${PLUGIN_NAME}_IS_ESSENTIAL YES)

include("starttls.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
