USE_ENCHANT {
  DEFINES       += HAVE_ENCHANT
  # To be fixed
  #win32 {
  #  INCLUDEPATH += $${ENCHANT_DEV_PATH}/include
  #  LIBS        += -L$${ENCHANT_DEV_PATH}/lib
  #  LIBS        += -lenchant 
  #}
  HEADERS       += enchantchecker.h
  SOURCES       += enchantchecker.cpp
}
