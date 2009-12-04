include(../config.inc)

TARGET             = vacuum
TEMPLATE           = app
QT                += xml
LIBS              += -L../libs
LIBS              += -lutils
DEPENDPATH        += ..
INCLUDEPATH       += ..
DESTDIR            = ../..
include(loader.pri)

#Appication icon
win32:RC_FILE      = loader.rc

#About Info
win32:system(subwcrev ../../. svninfo.tmpl svninfo.h > nul) {
  DEFINES         += SVNINFO
}

#Translation
TRANS_SOURCE_ROOT  = ..
include(../translations.inc)

#Install
include(../install.inc)
target.path        = $$INSTALL_BINS
translations.path  = $$INSTALL_TRANSLATIONS
translations.files = ../../translations/*
resources.path     = $$INSTALL_RESOURCES
resources.files    = ../../resources/*
INSTALLS           = target translations resources
