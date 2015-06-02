include(../config.inc)
include(../install.inc)

TARGET             = $$TARGET_LOADER
TEMPLATE           = app
QT                += xml
LIBS              += -L../libs
LIBS              += -l$$TARGET_UTILS
DEPENDPATH        += ..
INCLUDEPATH       += ..
DESTDIR            = ../..
include(loader.pri)

#Appication icon
win32:RC_FILE      = loader.rc

#GIT Info
GIT_HASH = $$system(git log -n 1 --format=%H)
GIT_DATE = $$system(git log -n 1 --format=%ct)
GIT_DATE = $$find(GIT_DATE,^\\d*)
!isEmpty(GIT_DATE) {
  win32 {
    WIN_OUT_PWD = $$replace(OUT_PWD, /, \\)
    system(mkdir $${WIN_OUT_PWD} & echo $${LITERAL_HASH}define GIT_HASH \"$${GIT_HASH}\" > $${WIN_OUT_PWD}\\gitinfo.h) {
      system(echo $${LITERAL_HASH}define GIT_DATE \"$${GIT_DATE}\" >> $${WIN_OUT_PWD}\\gitinfo.h)
      DEFINES         += GITINFO
      QMAKE_DISTCLEAN += $${OUT_PWD}/gitinfo.h
    }
  } else {
    system(mkdir -p $${OUT_PWD} && echo \\$${LITERAL_HASH}define GIT_HASH \\\"$${GIT_HASH}\\\" > $${OUT_PWD}/gitinfo.h) {
      system(echo \\$${LITERAL_HASH}define GIT_DATE \\\"$${GIT_DATE}\\\" >> $${OUT_PWD}/gitinfo.h)
      DEFINES         += GITINFO
      QMAKE_DISTCLEAN += $${OUT_PWD}/gitinfo.h
    }
  }
}

#Install
target.path        = $$INSTALL_BINS
resources.path     = $$INSTALL_RESOURCES
resources.files    = ../../resources/*
documents.path     = $$INSTALL_DOCUMENTS
documents.files    = ../../AUTHORS ../../CHANGELOG ../../README ../../COPYING ../../TRANSLATORS
INSTALLS           += target resources documents

#Translation
TRANS_SOURCE_ROOT  = ..
include(../translations.inc)

#Linux desktop install
unix:!macx {
  icons.path       = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/pixmaps
  icons.files      = ../../resources/menuicons/shared/vacuum.png
  INSTALLS        += icons

  desktop.path     = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/applications
  desktop.files    = ../../src/packages/linux/*.desktop
  INSTALLS        += desktop
}

#MacOS Install
macx {
  UTILS_LIB_NAME   = lib$${TARGET_UTILS}.$${VERSION_UTILS}.dylib
  UTILS_LIB_LINK   = lib$${TARGET_UTILS}.1.dylib
  UTILS_LIB_LINK_EXTERNAL_PLUGINS = lib$${TARGET_UTILS}.dylib

  lib_utils.path   = $$INSTALL_LIBS
  lib_utils.extra  = cp -f ../libs/$$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_LIBS/$$UTILS_LIB_NAME && \
                     ln -sf $$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_LIBS/$$UTILS_LIB_LINK && \
                     ln -sf $$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_LIBS/$$UTILS_LIB_LINK_EXTERNAL_PLUGINS
  INSTALLS        += lib_utils

  name_tool.path   = $$INSTALL_BINS
  name_tool.extra  = install_name_tool -change $$UTILS_LIB_LINK @executable_path/../Frameworks/$$UTILS_LIB_LINK $(INSTALL_ROOT)$$INSTALL_BINS/$$INSTALL_APP_DIR/Contents/MacOS/$$TARGET_LOADER
  INSTALLS        += name_tool

	#Dirty hack to install utils translations
	TARGET           = $$TARGET_UTILS
	include(../translations.inc)
	TARGET           = $$TARGET_LOADER

  ICON              = ../../vacuum.icns
  QMAKE_INFO_PLIST  = ../packages/macosx/Info.plist

  en_lproj.path     = $$INSTALL_RESOURCES/en.lproj/
  en_lproj.files    = ../packages/macosx/InfoPlist.strings
  INSTALLS         += en_lproj
}
