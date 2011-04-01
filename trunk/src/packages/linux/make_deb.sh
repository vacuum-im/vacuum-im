#!/bin/sh
# make_deb.sh


[ -e vacuum.pro ]|| {
	echo "You must execute this script from trunk directory."
	exit 1
}


echo "
Usage:
sh src/packages/linux/make_deb.sh

Build requirements: subversion build-essential devscripts fakeroot debhelper openssl libqtwebkit-dev libqt4-dev libx11-dev zlib1g-dev libidn11-dev cmake

Package file will be created in the parent directory
"


# Version
[ -d .svn ] && svn_version="$(sed -n -e '/^dir$/{n;p;q;}' .svn/entries 2>/dev/null)"||svn_version=""
# LC_ALL=C svn info 2> /dev/null | grep Revision | cut -d' ' -f2
#VER="$(echo `grep CLIENT_VERSION src/definitions/version.h|awk -F'"' '{print $2}'`|tr ' ' '.')"
VER="$(grep 'CLIENT_VERSION ' src/definitions/version.h|awk -F'"' '{print $2}')"
VERSION="${VER}+svn${svn_version}"


# Folders
sourcedir="${PWD}"
debdir="${sourcedir}/src/packages/linux/debian"
pkgdir="${sourcedir}/debian"


[ -d "${pkgdir}" ] && rm -rfv "${pkgdir}"
ln -s "${debdir}" "${pkgdir}" || exit 1
[ -f "${debdir}/changelog" ] && rm -vf "${debdir}/changelog"


DEBEMAIL="Sergey A Potapov <potapov.s.a@gmail.com>" dch --create --package vacuum --force-distribution -D "unstable" -v "${VERSION}-1" "Release" || exit 1
dpkg-buildpackage -b || exit 1


#Uncomment this part, if you want to clean compiled binaries
###################################################################
#make clean
#make distclean
#rm plugins/*so src/loader/svninfo.h
#find src/  -name '*.a' -exec rm -rvf '{}' \;
###################################################################


# Comment this part, if you want to to keep temporary files
###################################################################
dh_auto_clean
dh_clean
###################################################################


echo "All Done"
exit 0
