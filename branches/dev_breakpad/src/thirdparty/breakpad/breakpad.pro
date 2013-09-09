include(../../make/config.inc)

TARGET       = breakpad
TEMPLATE     = lib
CONFIG      -= qt
CONFIG      += staticlib warn_off
INCLUDEPATH += .
DESTDIR      = ../../libs
include(breakpad.pri)
