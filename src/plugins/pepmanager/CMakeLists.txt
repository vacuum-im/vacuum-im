project(pepmanager)

set(PLUGIN_NAME "pepmanager")
set(PLUGIN_DISPLAY_NAME "PEP manager")
set(PLUGIN_DEPENDENCIES stanzaprocessor servicediscovery xmppstreams) # used only in CPack

include("pepmanager.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
