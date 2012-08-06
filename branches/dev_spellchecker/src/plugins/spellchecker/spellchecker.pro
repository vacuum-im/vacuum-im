#Plugin file name
TARGET              = spellchecker
include(config.inc)

#Project Configuration
TEMPLATE            = lib
CONFIG             += plugin
QT                  = core gui xml
LIBS               += -l$${TARGET_UTILS}
LIBS               += -L$${VACUUM_LIB_PATH}
DEPENDPATH         += $${VACUUM_SRC_PATH}
INCLUDEPATH        += $${VACUUM_SRC_PATH}

#Include Backends
include(enchantchecker.inc)
include(aspellchecker.inc)
#include(macspellchecker.inc)
include(hunspellchecker.inc)

#Install
include(install.inc)

#Translation
include(translations.inc)

include(spellchecker.pri)
