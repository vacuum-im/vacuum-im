TARGET = zlib1
TEMPLATE = lib
CONFIG -= qt
CONFIG += dll warn_off
DEFINES += ZLIB_DLL
DLLDESTDIR = ../../..
DESTDIR = ../../libs
include(zlib.pri)