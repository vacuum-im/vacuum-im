project(jabbersearch)

set(PLUGIN_NAME "jabbersearch")
set(PLUGIN_DISPLAY_NAME "Jabber search")
set(PLUGIN_DEPENDENCIES stanzaprocessor) # used only in CPack

include("jabbersearch.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
