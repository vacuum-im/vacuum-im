project(annotations)

set(PLUGIN_NAME "annotations")
set(PLUGIN_DISPLAY_NAME "Annotations")
set(PLUGIN_DEPENDENCIES privatestorage) # used only in CPack

include("annotations.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
