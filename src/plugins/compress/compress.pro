TARGET         = compress 
unix:!macx {
  LIBS        += -lz
} else {
  LIBS        += -lzlib
  INCLUDEPATH += ../../thirdparty/zlib
}
  
include(compress.pri)
include(../plugins.inc)