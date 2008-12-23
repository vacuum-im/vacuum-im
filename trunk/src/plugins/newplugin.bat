mkdir %1
cd %1

echo TARGET = %1 >> %1.pro
echo include(%1.pri) >> %1.pro
echo include(../plugins.inc) >> %1.pro

echo HEADERS = >> %1.pri
echo SOURCES = >> %1.pri

lupdate %1.pro

cd ..
make_vcproj.bat %1