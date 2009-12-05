include(../config.inc)

TEMPLATE          = lib
CONFIG           += plugin
QT               += network xml
LIBS             += -L../../libs
LIBS             += -lvacuumutils
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
