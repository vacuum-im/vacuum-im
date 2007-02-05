TEMPLATE = lib

CONFIG += dll
CONFIG -= qt

DEFINES += ZLIB_DLL

DLLDESTDIR = ../../..

DESTDIR = ../../libs

TARGET = zlib1

include(zlib.pri)