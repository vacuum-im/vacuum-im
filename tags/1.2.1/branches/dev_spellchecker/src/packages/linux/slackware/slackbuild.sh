#!/bin/bash

#
# Author for slack build script script Oleg A Deordiev kvantarium@gmail.com
#

arche=$HOSTTYPE
CWD="$(pwd)"
startdir="$CWD"
srcdir="$startdir"
builddir="$srcdir"/build
pkgdir="$srcdir"/pkgdir
instldir="$srcdir"/instldir
cpucore=""
threadsmy=""
pkgtype=txz
revpkg="1"
CLIENT_NAME="vacuum-im"
CLIENT_VERSION=""
CLIENT_VERSION_SUFIX=""
CLIENT_SVN_VER=""

# Go to root
cd ../../../../

# If no correct position
if [ ! -d ./src ]
then
	echo "Not found dir src, maybe not correct folder"
	exit 1
fi

# Patch for version OS
patch -p0 < src/packages/linux/slackware/clientinfo.patch

CWD="$(pwd)"
srcdir="$CWD"
builddir="$srcdir"/build
pkgdir="$srcdir"/pkgdir
instldir="$srcdir"/instldir

CLIENT_VERSION="$(grep -m 1 "CLIENT_VERSION" ./src/definitions/version.h | awk -F '"' '{print $2}' | awk -F '"' '{print $1}')"
CLIENT_VERSION_SUFIX="$(grep "CLIENT_VERSION_SUFIX" ./src/definitions/version.h | awk -F '"' '{print $2}' | awk -F '"' '{print $1}')"

if [ -d "$srcdir"/.svn ]
then
	echo "This is a svn version"
	# Get svn version
	CLIENT_SVN_VER="$(sed -n -e '/^dir$/{n;p;q;}' .svn/entries)"
fi

# Check dir for cmake
if [ ! -d "$instldir" ]
then
	mkdir -p "$instldir"
fi

# Go to cmake dir
cd "$instldir"

# Run cmake
if [ "$arche" == "x86_64" ]
then
	echo "Compile for "$arche" architecture"
	if ! cmake -DCMAKE_INSTALL_PREFIX=/usr -DINSTALL_LIB_DIR=lib64 .. > /dev/null 2>&1
		then
		echo "Eror - cmake"
		exit 1
	fi
else
	echo "Compile for "$arche" architecture"
	if ! cmake -DCMAKE_INSTALL_PREFIX=/usr -DINSTALL_LIB_DIR=lib .. > /dev/null 2>&1
	then
		echo "Error - cmake"
		exit 1
	fi
fi

# Get cpu number
cpucore="$(cat /proc/cpuinfo | grep -m 1 "cpu cores" | awk -F": " '{print $2}')"

# Check CPU function
if [ "$cpucore" == "" ]
then
	echo "Failed to get CPU number, maybe have only 1 core"
	echo "Set CPU number to 1"
	cpucore=1
fi

echo "Make program in "$cpucore" CPU"

# Make
if ! make -j "$cpucore" > /dev/null 2>&1
then
	echo "Erorr - make"
	echo "Make a log file, please wait..."
	make -j "$cpucore" > ./makefile.log 2>&1
	tail ./makefile.log
	exit 1
fi

if [ ! -d $builddir ]
then
	mkdir -p $builddir
fi

# Install to fake root dir
if ! make install DESTDIR="$builddir" > /dev/null 2>&1
then
	echo "Error - make install"
	exit 1
fi

# Go to complited version
cd $builddir

# Create dir for slackware package manager
mkdir install

# Include special files for slackpkg
cp "$startdir"/slack-desc ./install/slack-desc
cp "$startdir"/slack-required ./install/slack-required

# Create dir for complited package
if [ ! -d "$pkgdir" ]
then
	mkdir -p "$pkgdir"
fi

# Create package for Slackware Linux
if makepkg -l y -p -c y "$pkgdir"/"$CLIENT_NAME"-"$CLIENT_VERSION""$CLIENT_VERSION_SUFIX""$CLIENT_SVN_VER"-"$arche"-"$revpkg"."$pkgtype" > /dev/null 2>&1
then
	echo "Package created - "$pkgdir"/"$CLIENT_NAME"-"$CLIENT_VERSION""$CLIENT_VERSION_SUFIX""$CLIENT_SVN_VER"-"$arche"-"$revpkg"."$pkgtype""
else
	echo "Package create FAIL"
	exit 1
fi

cd "$srcdir"

# CleanUp
rm -rf "$builddir"
rm -rf "$instldir"
echo "Clean"

exit 0
