project(bitsofbinary)

set(PLUGIN_NAME "bitsofbinary")
set(PLUGIN_DISPLAY_NAME "Bits of binary")
set(PLUGIN_DEPENDENCIES stanzaprocessor) # used only in CPack

include("bitsofbinary.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
