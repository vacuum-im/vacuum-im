#!/bin/bash

VERSION=0.0.0.Preview
DEPENDS='openssl, libqt4-network (>= 4.5.2), libqt4-webkit (>= 4.5.2), libqt4-xml (>= 4.5.2), libqtcore4 (>= 4.5.2), libqtgui4 (>= 4.5.2)'
MAINTAINER='Sergey A Potapov <potapov.s.a@gmail.com>'

startdir=$PWD
pkgdir=${PWD}/pkg

case `uname -m` in
    i[3-6]86) arch='i386';;
    x86_64|amd64) arch='amd64';;
    *) echo "Unknown architecture."; exit 1
esac


# build
cd ../../.. || exit 1

qmake INSTALL_PREFIX=/usr -recursive vacuum.pro || exit 1
make || exit 1

mkdir -p $pkgdir
make INSTALL_ROOT=${pkgdir} install || exit 1


# vacuum.desktop & icons

install -Dm644 ${startdir}/vacuum.desktop \
    ${pkgdir}/usr/share/applications/vacuum.desktop || exit 1

mkdir -p ${pkgdir}/usr/share/icons/hicolor/16x16/apps/
cd ${pkgdir}/usr/share/icons/hicolor/16x16/apps/
ln -s ../../../../vacuum/resources/menuicons/shared/mainwindowlogo16.png vacuum.png || exit 1

mkdir -p ${pkgdir}/usr/share/icons/hicolor/24x24/apps/
cd ${pkgdir}/usr/share/icons/hicolor/24x24/apps/
ln -s ../../../../vacuum/resources/menuicons/shared/mainwindowlogo24.png vacuum.png || exit 1

mkdir -p ${pkgdir}/usr/share/icons/hicolor/32x32/apps/
cd ${pkgdir}/usr/share/icons/hicolor/32x32/apps/
ln -s ../../../../vacuum/resources/menuicons/shared/mainwindowlogo32.png vacuum.png || exit 1

mkdir -p ${pkgdir}/usr/share/icons/hicolor/48x48/apps/
cd ${pkgdir}/usr/share/icons/hicolor/48x48/apps/
ln -s ../../../../vacuum/resources/menuicons/shared/mainwindowlogo48.png vacuum.png || exit 1

mkdir -p ${pkgdir}/usr/share/icons/hicolor/64x64/apps/
cd ${pkgdir}/usr/share/icons/hicolor/64x64/apps/
ln -s ../../../../vacuum/resources/menuicons/shared/mainwindowlogo64.png vacuum.png || exit 1

mkdir -p ${pkgdir}/usr/share/icons/hicolor/96x96/apps/
cd ${pkgdir}/usr/share/icons/hicolor/96x96/apps/
ln -s ../../../../vacuum/resources/menuicons/shared/mainwindowlogo96.png vacuum.png || exit 1

mkdir -p ${pkgdir}/usr/share/icons/hicolor/128x128/apps/
cd ${pkgdir}/usr/share/icons/hicolor/128x128/apps/
ln -s ../../../../vacuum/resources/menuicons/shared/mainwindowlogo128.png vacuum.png || exit 1

cd $startdir || exit 1


# make control file
installed_size=`du -s $pkgdir | awk '{print $1}'`
mkdir -p $pkgdir/DEBIAN

echo "Package: vacuum
Version: $VERSION
Architecture: $arch
Maintainer: $MAINTAINER
Installed-Size: $installed_size
Depends: $DEPENDS
Section: net
Priority: optional
Homepage: http://www.vacuum-im.org/
Description: Vacuum IM - Qt based crossplatform Jabber client" > $pkgdir/DEBIAN/control || exit 1


# make deb package
dpkg-deb -b $pkgdir $startdir || exit 1


# clear
rm -rf $pkgdir

echo "All Done."
exit 0