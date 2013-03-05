project(bookmarks)

set(PLUGIN_NAME "bookmarks")
set(PLUGIN_DISPLAY_NAME "Bookmarks")
set(PLUGIN_DEPENDENCIES privatestorage) # used only in CPack

include("bookmarks.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
