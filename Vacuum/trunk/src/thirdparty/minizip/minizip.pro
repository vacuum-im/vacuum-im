TARGET = minizip
TEMPLATE = lib
CONFIG -= qt
CONFIG += staticlib warn_off
DEFINES += ZLIB_DLL
LIBS += -l../../libs/zlib1
INCLUDEPATH += ../zlib
DESTDIR = ../../libs
include(minizip.pri)
