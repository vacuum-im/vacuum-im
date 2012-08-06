USE_ASPELL {
  DEFINES       += HAVE_ASPELL
  win32 {
    INCLUDEPATH += $${ASPELL_DEV_PATH}/include
    LIBS        += -L$${ASPELL_DEV_PATH}/lib
    LIBS        += -laspell-15 
  }
  HEADERS       += aspellchecker.h
  SOURCES       += aspellchecker.cpp
}
