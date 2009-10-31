TARGET       = vacuum
TEMPLATE     = app
QT          += xml
LIBS        += -L../libs
LIBS        += -lutils
INCLUDEPATH  = ..
DESTDIR      = ../..
include(loader.pri)

#Translations
TRANS_SOURCE_ROOT = ..
include(../translations.inc)