include(../config.inc)

TARGET             = vacuumutils
TEMPLATE           = lib
CONFIG            += dll
QT                += xml
DEFINES           += UTILS_DLL
LIBS              += -L../libs
LIBS              += -lidn -lminizip -lzlib
DEPENDPATH        += ..
INCLUDEPATH       += ..
DESTDIR            = ../libs
DLLDESTDIR         = ../..
include(utils.pri)

#Translation
TRANS_SOURCE_ROOT  = ..
include(../translations.inc)

#Install
include(../install.inc)
target.path        = $$INSTALL_LIBS
INSTALLS           = target
