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

#Unix Install
unix {
  LD_CONFIG_PATH   = /etc/ld.so.conf.d
  LD_CONFIG_FILE   = $${LD_CONFIG_PATH}/$${INSTALL_DIR}.conf

  ld_config.path   = $$LD_CONFIG_PATH
  ld_config.extra  = rm -f $$LD_CONFIG_FILE && \
                     echo $${INSTALL_LIBS} >> $$LD_CONFIG_FILE && \
                     ldconfig
  INSTALLS        += ld_config
}

#MaxOS Install
macx {
  UTILS_LIB_NAME   = lib$${TARGET_UTILS}.1.0.0.dylib
  UTILS_LIB_LINK   = lib$${TARGET_UTILS}.1.dylib

  lib_utils.path   = $$INSTALL_LIBS
  lib_utils.extra  = cp -f ../libs/$$TARGET_UTILS/$$UTILS_LIB_NAME $$INSTALL_LIBS/$$UTILS_LIB_NAME && \
                     ln -sf $$UTILS_LIB_NAME $$INSTALL_LIBS/$$UTILS_LIB_LINK
  INSTALLS        += lib_utils

  name_tool.path   = $$INSTALL_BINS
  name_tool.extra  = install_name_tool -change $$UTILS_LIB_LINK @executable_path/../Frameworks/$$UTILS_LIB_LINK $$INSTALL_BINS/$$INSTALL_DIR/Contents/MacOS/$$TARGET_LOADER
  INSTALLS        += name_tool
}
