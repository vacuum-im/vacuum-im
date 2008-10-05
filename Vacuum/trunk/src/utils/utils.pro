TARGET = utils
TEMPLATE = lib
CONFIG += dll
QT += xml
LIBS += -L../libs
LIBS += -lidn -lminizip
DEFINES += UTILS_DLL
DLLDESTDIR = ../..
DESTDIR = ../libs
include(utils.pri)

#Translations
include(../translations.inc)