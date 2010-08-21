project(pep)

set(PLUGIN_NAME "pep")
set(PLUGIN_DISPLAY_NAME "PEP manager")
set(PLUGIN_DEPENDENCIES stanzaprocessor servicediscovery xmppstreams) # used only in CPack

include("pep.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")
