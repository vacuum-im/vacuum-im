mkdir %1
cd %1

echo include(../plugins.inc) >> %1.pro
echo QT += network xml >> %1.pro
echo LIBS += -l../../libs/utils >> %1.pro
echo TARGET = %1 >> %1.pro
echo include(%1.pri) >> %1.pro

echo HEADERS = %1.h>> %1.pri
echo SOURCES = %1.cpp>> %1.pri

echo #ifndef %1_H >> %1.h
echo #define %1_H >> %1.h
echo #endif >> %1.h
echo #include "%1.h" >> %1.cpp

qmake -tp vc -o %1.vcproj %1.pro 