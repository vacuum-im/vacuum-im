include(../../make/config.inc)

TARGET         = qxtglobalshortcut
TEMPLATE       = lib
CONFIG        += staticlib warn_off
DESTDIR        = ../../libs
DEFINES       += QXT_STATIC
QT            += widgets
INCLUDEPATH   += ../..
include(qxtglobalshortcut.pri)
