include(../config.inc)

TEMPLATE          = lib
CONFIG           += plugin
QT               += network xml
LIBS             += -L../../libs
LIBS             += -l$$TARGET_UTILS
DEPENDPATH       += ../..
INCLUDEPATH      += ../..
win32:DLLDESTDIR  = ../../../plugins
unix:DESTDIR      = ../../../plugins

#Translation
TRANS_SOURCE_ROOT = ../..
include(../translations.inc)

#Install
include(../install.inc)
target.path       = $$INSTALL_PLUGINS
INSTALLS          = target

#MaxOS Install
macx {
  name_tool.path  = $$INSTALL_PLUGINS
  name_tool.extra = install_name_tool -change lib$${TARGET_UTILS}.1.dylib @executable_path/../Frameworks/lib$${TARGET_UTILS}.1.dylib $$INSTALL_PLUGINS/lib$${TARGET}.dylib
  INSTALLS       += name_tool
}
