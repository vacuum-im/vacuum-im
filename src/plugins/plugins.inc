TEMPLATE       = lib
CONFIG        += plugin
QT            += network xml
LIBS          += -L../../libs
LIBS          += -lutils
win32:{
  DLLDESTDIR   = ../../../plugins
}
unix:{
  DESTDIR      = ../../../plugins
}

#Translations
TRANS_SOURCE_ROOT = ../..
include(../translations.inc)