include(../config.inc)
include(../install.inc)

TARGET             = $$TARGET_UTILS
TEMPLATE           = lib
VERSION            = $$VERSION_UTILS
CONFIG            += dll
QT                += xml
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
  QMAKE_LFLAGS    += -framework Carbon
} else:unix {
  LIBS            += -lXss
  CONFIG          += x11
} else:win32 {
  LIBS            += -luser32
}

include(utils.pri)

#Translation
TRANS_SOURCE_ROOT  = ..
include(../translations.inc)

#Install (for Mac OS X - in loader.pro)
!macx:{
  target.path      = $$INSTALL_LIBS
  INSTALLS        += target
}
