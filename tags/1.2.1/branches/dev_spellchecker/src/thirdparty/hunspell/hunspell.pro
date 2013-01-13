include(../../config.inc)

TARGET   = hunspell
TEMPLATE = lib
CONFIG  -= qt
CONFIG  += staticlib warn_off
DEFINES += HUNSPELL_STATIC
DESTDIR  = ../../libs
include(hunspell.pri)
