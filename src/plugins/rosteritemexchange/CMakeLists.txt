project(rosteritemexchange)

set(PLUGIN_NAME "rosteritemexchange")
set(PLUGIN_DISPLAY_NAME "Roster Item Exchange")
set(PLUGIN_DEPENDENCIES roster stanzaprocessor) # used only in CPack

include("rosteritemexchange.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
