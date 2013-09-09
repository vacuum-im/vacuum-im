project(shortcutmanager)

set(PLUGIN_NAME "shortcutmanager")
set(PLUGIN_DISPLAY_NAME "Shortcut Manager")
set(PLUGIN_DEPENDENCIES optionsmanager) # used only in CPack

include("shortcutmanager.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
