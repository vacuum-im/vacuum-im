include(../../config.inc)

TARGET         = minizip
TEMPLATE       = lib
CONFIG        -= qt
CONFIG        += staticlib warn_off
INCLUDEPATH   += ../zlib
DESTDIR        = ../../libs
include(minizip.pri)
