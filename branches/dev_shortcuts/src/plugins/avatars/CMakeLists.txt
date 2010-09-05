project(avatars)

set(PLUGIN_NAME "avatars")
set(PLUGIN_DISPLAY_NAME "Avatars")
set(PLUGIN_DEPENDENCIES vcard) # used only in CPack

include("avatars.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
