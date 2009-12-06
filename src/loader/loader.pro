include(../config.inc)

TARGET             = $$TARGET_LOADER
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
win32 {
  exists(svninfo.h):system(del svninfo.h)
  !isEmpty(SVN_REVISION):system(echo $${LITERAL_HASH}define SVN_REVISION \"$$SVN_REVISION\" >> svninfo.h) {
    DEFINES       += SVNINFO
  }
} else {
  exists(svninfo.h):system(rm svninfo.h)
  !isEmpty(SVN_REVISION):system(echo \\$${LITERAL_HASH}define SVN_REVISION \\\"$${SVN_REVISION}\\\" >> svninfo.h) {
    DEFINES       += SVNINFO
  }
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

#MaxOS Changes
macx {
  name_tool.path   = $$INSTALL_BINS
  name_tool.extra  = install_name_tool -change lib$${TARGET_UTILS}.1.dylib @executable_path/../Frameworks/lib$${TARGET_UTILS}.1.dylib $$INSTALL_BINS/$$INSTALL_DIR/Contents/MacOS/$$TARGET_LOADER
  INSTALLS        += name_tool
}
