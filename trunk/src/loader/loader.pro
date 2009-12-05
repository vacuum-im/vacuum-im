include(../config.inc)

TARGET             = vacuum
TEMPLATE           = app
QT                += xml
LIBS              += -L../libs
LIBS              += -lvacuumutils
DEPENDPATH        += ..
INCLUDEPATH       += ..
DESTDIR            = ../..
include(loader.pri)

#Appication icon
win32:RC_FILE      = loader.rc

#SVN Info
SVN_REVISION=$$system(svnversion -n)
exists(svninfo.h) {
  win32 {
    system(del svninfo.h)
  } else {
    system(rm svninfo.h)
  }
}
!isEmpty(SVN_REVISION):system(echo $${LITERAL_HASH}define SVN_REVISION \"$$SVN_REVISION\" >> svninfo.h) {
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
