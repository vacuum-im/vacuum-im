project(privatestorage)

set(PLUGIN_NAME "privatestorage")
set(PLUGIN_DISPLAY_NAME "Private storage")
set(PLUGIN_DEPENDENCIES stanzaprocessor) # used only in CPack

include("privatestorage.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
