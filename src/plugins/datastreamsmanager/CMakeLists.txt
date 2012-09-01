project(datastreamsmanager)

set(PLUGIN_NAME "datastreamsmanager")
set(PLUGIN_DISPLAY_NAME "Data streams manager")
set(PLUGIN_DEPENDENCIES dataforms stanzaprocessor) # used only in CPack

include("datastreamsmanager.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
