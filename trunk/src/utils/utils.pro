include(../config.inc)
include(../install.inc)

TARGET             = $$TARGET_UTILS
TEMPLATE           = lib
VERSION            = $$VERSION_UTILS
CONFIG            += dll
QT                += xml
DEFINES           += UTILS_DLL QXT_STATIC
LIBS              += -L../libs
LIBS              += -lidn -lminizip -lqxtglobalshortcut -lidle
macx {
  LIBS            += -lzlib
  INCLUDEPATH     += ../thirdparty/zlib
} else:unix {
  LIBS            += -lz
  CONFIG          += x11
} else:win32 {
  LIBS            += -lzlib -luser32
  INCLUDEPATH     += ../thirdparty/zlib
}
DEPENDPATH        += ..
INCLUDEPATH       += ..
win32 {
  DLLDESTDIR       = ..\\..
  QMAKE_DISTCLEAN += $${DLLDESTDIR}\\$${TARGET}.dll
}
DESTDIR            = ../libs
include(utils.pri)

#Translation
TRANS_SOURCE_ROOT  = ..
include(../translations.inc)

#Install (for Mac OS X - in loader.pro)
!macx:{
  target.path      = $$INSTALL_LIBS
  INSTALLS        += target
}
