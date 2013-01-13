project(registration)

set(PLUGIN_NAME "registration")
set(PLUGIN_DISPLAY_NAME "Registration")
set(PLUGIN_DEPENDENCIES dataforms stanzaprocessor) # used only in CPack

include("registration.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
