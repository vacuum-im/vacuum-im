TEMPLATE = lib

CONFIG += dll warn_off
CONFIG -= qt

DEFINES += ZLIB_DLL

DLLDESTDIR = ../../..

DESTDIR = ../../libs

TARGET = zlib1

include(zlib.pri)