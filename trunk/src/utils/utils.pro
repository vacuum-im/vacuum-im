include(../config.inc)

TARGET             = $$TARGET_UTILS
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
!macx:target.path  = $$INSTALL_LIBS
macx:target.path   = ../libs/$$TARGET_UTILS    #Install in loader.pro
INSTALLS           = target
