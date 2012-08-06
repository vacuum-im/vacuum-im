#Path to copy plugin on build
win32:DLLDESTDIR       = ../vacuum/plugins
unix:DESTDIR           = ../vacuum/plugins

#Path from vacumm to plugin resources
DEFINES               += RESOURCES_DIR='\\\"../vacuum-$${TARGET}/resources\\\"'
