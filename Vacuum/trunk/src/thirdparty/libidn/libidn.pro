TEMPLATE = lib

CONFIG += staticlib warn_off
CONFIG -= qt

DESTDIR = ../../libs 

TARGET = idn

include(libidn.pri) 