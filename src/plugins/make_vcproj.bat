set QTDIR=c:\Qt5\5.7\msvc2015
set QMAKESPEC=win32-msvc2015
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

cd %1
%QTDIR%\bin\qmake.exe -spec $(QTDIR)\mkspecs\$(QMAKESPEC) QMAKE_INCDIR_QT=$(QTDIR)\include QMAKE_LIBDIR=$(QTDIR)\lib QMAKE_UIC=$(QTDIR)\bin\uic.exe QMAKE_MOC=$(QTDIR)\bin\moc.exe QMAKE_RCC=$(QTDIR)\bin\rcc.exe QMAKE_QMAKE=$(QTDIR)\bin\qmake.exe -tp vc -o %1.vcxproj %1.pro 

pause
