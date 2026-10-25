project(imageviewer)

set(PLUGIN_NAME "imageviewer")
set(PLUGIN_DISPLAY_NAME "ImageViewer")
set(PLUGIN_DEPENDENCIES messagewidgets) # used only in CPack
set(ADD_LIBS Qt5::Concurrent)

include("imageviewer.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
