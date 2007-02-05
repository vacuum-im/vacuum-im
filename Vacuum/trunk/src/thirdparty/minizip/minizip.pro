TEMPLATE = lib

CONFIG += staticlib
CONFIG -= qt

DEFINES += ZLIB_DLL

LIBS += -l../../libs/zlib1

INCLUDEPATH += ../zlib

DESTDIR = ../../libs

TARGET = minizip
 
include(minizip.pri)
