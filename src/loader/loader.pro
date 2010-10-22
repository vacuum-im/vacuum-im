include(../config.inc)

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
macx:ICON          = ../../vacuum.icns

#SVN Info
SVN_REVISION=$$system(svnversion -n -c ./../../)
win32 {
  exists(svninfo.h):system(del svninfo.h)
  !isEmpty(SVN_REVISION):system(echo $${LITERAL_HASH}define SVN_REVISION \"$$SVN_REVISION\" >> svninfo.h) {
    DEFINES       += SVNINFO
  }
} else {
  exists(svninfo.h):system(rm -f svninfo.h)
  !isEmpty(SVN_REVISION):system(echo \\$${LITERAL_HASH}define SVN_REVISION \\\"$${SVN_REVISION}\\\" >> svninfo.h) {
    DEFINES       += SVNINFO
  }
}

#Translation
TRANS_SOURCE_ROOT  = ..
include(../translations.inc)

#Qt Translations
#qt_qm_all.target = build_qt_qm
#for(LANG, TRANS_LANGS) {
#  SHORT_LANG = $${LANG}
#  SHORT_LANG ~= s/??_??/ru
#
#  TS_FILE = $$[QT_INSTALL_TRANSLATIONS]/qt_$${SHORT_LANG}.ts
#  QM_FILE = $${TRANS_SOURCE_ROOT}/../translations/$${LANG}/qt_$${SHORT_LANG}.qm
#
#  LRELEASE = $$[QT_INSTALL_BINS]/lrelease
#  win32: LRELEASE = $$replace(LRELEASE, "/", "\\")
#
#  eval(qt_qm_$${LANG}.target   = $${QM_FILE})
#  eval(qt_qm_$${LANG}.depends  = $${TS_FILE})
#  eval(qt_qm_$${LANG}.commands = $${LRELEASE} -compress $${TS_FILE} -qm $${QM_FILE})
#
#  qt_qm_all.depends    += qt_qm_$${SHORT_LANG}
#  QMAKE_EXTRA_TARGETS  += qt_qm_$${SHORT_LANG}
#}
#QMAKE_EXTRA_TARGETS  += qt_qm_all
#POST_TARGETDEPS      += $${qt_qm_all.target}

#Install
include(../install.inc)
target.path        = $$INSTALL_BINS
resources.path     = $$INSTALL_RESOURCES
resources.files    = ../../resources/*
documents.path     = $$INSTALL_DOCUMENTS
documents.files    = ../../AUTHORS ../../CHANGELOG ../../README ../../COPYING ../../TRANSLATORS
translations.path  = $$INSTALL_TRANSLATIONS
translations.files = ../../translations/*
INSTALLS           = target resources documents translations

#Linux desktop install
unix:!macx {
  icons.path       = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/pixmaps
  icons.files      = ../../resources/menuicons/shared/vacuum.png
  INSTALLS        += icons

  desktop.path     = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/applications
  desktop.files    = ../../src/packages/linux/*.desktop
  INSTALLS        += desktop
}

#MaxOS Install
macx {
  UTILS_LIB_NAME   = lib$${TARGET_UTILS}.1.0.0.dylib
  UTILS_LIB_LINK   = lib$${TARGET_UTILS}.1.dylib

  lib_utils.path   = $$INSTALL_LIBS
  lib_utils.extra  = cp -f ../libs/$$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_LIBS/$$UTILS_LIB_NAME && \
                     ln -sf $$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_LIBS/$$UTILS_LIB_LINK
  INSTALLS        += lib_utils

  name_tool.path   = $$INSTALL_BINS
  name_tool.extra  = install_name_tool -change $$UTILS_LIB_LINK @executable_path/../Frameworks/$$UTILS_LIB_LINK $(INSTALL_ROOT)$$INSTALL_BINS/$$INSTALL_APP_DIR/Contents/MacOS/$$TARGET_LOADER
  INSTALLS        += name_tool
}
