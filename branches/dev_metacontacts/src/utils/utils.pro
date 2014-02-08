include(../make/config.inc)

TARGET             = $$VACUUM_UTILS_NAME
TEMPLATE           = lib
VERSION            = $$VACUUM_UTILS_ABI
CONFIG            += dll
QT                += xml network
DEFINES           += UTILS_DLL QXT_STATIC

DEPENDPATH        += ..
INCLUDEPATH       += ..

DESTDIR            = ../libs
win32 {
  DLLDESTDIR       = ..\\..
  QMAKE_DISTCLEAN += $${DLLDESTDIR}\\$${TARGET}.dll
}

LIBS              += -L../libs
LIBS              += -lqxtglobalshortcut -lidle -lidn -lminizip -lzlib
macx {
  QMAKE_LFLAGS    += -framework Carbon -framework IOKit -framework Cocoa
} else:unix:!haiku {
  LIBS            += -lXss
  CONFIG          += x11
} else:win32 {
  LIBS            += -luser32
}

include(utils.pri)

#Install (for MacOS in loader.pro because instalation directory will be removed in loader installer)
!macx:{
  target.path      = $$INSTALL_LIBS
  INSTALLS        += target

  sdk_utils.path   = $$INSTALL_INCLUDES/utils
  sdk_utils.files  = *.h
  INSTALLS        += sdk_utils

  #Translation
  TRANS_BUILD_ROOT   = $${OUT_PWD}/../..
  TRANS_SOURCE_ROOT  = ..
  include(../translations/languages.inc)
}
