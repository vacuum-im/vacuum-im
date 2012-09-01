include(../config.inc)
include(../install.inc)

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
LIBS              += -lzlib -lidn -lminizip -lqxtglobalshortcut -lidle
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

  #Translation
  TRANS_SOURCE_ROOT  = ..
  include(../translations.inc)
}
