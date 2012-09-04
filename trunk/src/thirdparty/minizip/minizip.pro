include(../../sdk/config.inc)

TARGET         = minizip
TEMPLATE       = lib
CONFIG        -= qt
CONFIG        += staticlib warn_off
INCLUDEPATH   += ../..
DESTDIR        = ../../libs
include(minizip.pri)
