TARGET       = vacuum
TEMPLATE     = app
QT          += xml
LIBS        += -L../libs
LIBS        += -lutils
DEPENDPATH  += ..
INCLUDEPATH += ..
DESTDIR      = ../..
include(loader.pri)

#Appication icon
win32:RC_FILE = loader.rc

#Translations
TRANS_SOURCE_ROOT = ..
include(../translations.inc)

#About Info
win32:system(subwcrev ../../. svninfo.tmpl svninfo.h > nul) {
  DEFINES += SVNINFO
}
