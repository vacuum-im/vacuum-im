TARGET       = client
TEMPLATE     = app
INCLUDEPATH  = ..
DESTDIR      = ../..
include(client.pri)

#Translations
TRANS_SOURCE_ROOT = ..
include(../translations.inc)