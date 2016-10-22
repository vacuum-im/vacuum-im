include(../make/config.inc)

TARGET             = $$VACUUM_LOADER_NAME
TEMPLATE           = app
QT                += widgets xml
LIBS              += -L../libs
LIBS              += -l$$VACUUM_UTILS_NAME
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
TRANS_BUILD_ROOT   = $${OUT_PWD}/../..
TRANS_SOURCE_ROOT  = ..
include(../translations/languages.inc)

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
  UTILS_LIB_NAME   = lib$${VACUUM_UTILS_NAME}.$${VACUUM_UTILS_ABI}.dylib
  UTILS_LIB_LINK   = lib$${VACUUM_UTILS_NAME}.dylib

  lib_utils.path   = $$INSTALL_LIBS
  lib_utils.extra  = cp -f ../libs/$$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_LIBS/$$UTILS_LIB_NAME && \
                     ln -sf $$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_LIBS/$$UTILS_LIB_LINK
  INSTALLS        += lib_utils

  name_tool.path   = $$INSTALL_BINS
  name_tool.extra  = install_name_tool -change $$UTILS_LIB_NAME @executable_path/../Frameworks/$$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_BINS/$$INSTALL_APP_DIR/Contents/MacOS/$$VACUUM_LOADER_NAME
  INSTALLS        += name_tool

  sdk_utils.path   = $$INSTALL_INCLUDES/utils
  sdk_utils.files  = ../utils/*.h
  INSTALLS        += sdk_utils

  #Dirty hack to install utils translations
  #TARGET           = $$VACUUM_UTILS_NAME
  #include(../translations/languages.inc)
  #TARGET           = $$VACUUM_LOADER_NAME

  ICON              = ../../vacuum.icns
  QMAKE_INFO_PLIST  = ../packages/macosx/Info.plist

  en_lproj.path     = $$INSTALL_RESOURCES/en.lproj/
  en_lproj.files    = ../packages/macosx/InfoPlist.strings
  INSTALLS         += en_lproj
}
