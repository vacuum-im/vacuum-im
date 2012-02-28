#!/bin/bash
#
# make_dmg.sh - Add Qt libraries to Mac OS X bundle and pack it to .dmg image
# Copyright (C) 2010 Konstantin Tseplyaev (for Vacuum-IM project by Sergey Potapov)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# RUN THIS SCRIPT FROM SOURCE ROOT DIRECTORY - "sh src/packages/macosx/make_dmg.sh"

# This script assumes that you already hame a bundle
# with all application-related plugins and libraries in it,
# for example, libvacuumutils.xx.dylib

# Known issues:
# 1. On some Qt installations can produce bundle that tries to load
#    some Qt libraries from system folders.


# Version
[ -d .svn ] && REVISION=".$(sed -n -e '/^dir$/{n;p;q;}' .svn/entries 2>/dev/null)"||REVISION=""
VER_NUMBER="$(grep 'CLIENT_VERSION ' src/definitions/version.h|awk -F'"' '{print $2}')"
VERSION="${VER_NUMBER}${REVISION}"

# Name of the original bundle
ORIG_NAME="vacuum" 

# Source tree root
SVN_ROOT="../../.." 

# Name of bundle, as it'll be included in .dmg
PRODUCT_NAME="Vacuum-IM"

# Filename of the resulting .dmg image
DMG_NAME="${ORIG_NAME}_${VERSION}_macosx" 

# IMPORTANT! Path to qmake of Qt installation, against which project was built.
PATH_TO_QMAKE="qmake"

# Build options
BUILDOPTS=" CONFIG+=release CONFIG+=x86_64"

# Directories
TMP_DIR="./Applications"
DYLD_PREFIX="@executable_path/../Frameworks"
CONTENTS_DIR="$TMP_DIR/$ORIG_NAME.app/Contents"
FW_DIR="$CONTENTS_DIR/Frameworks"
SYS_PLUGINS_DIR=`$PATH_TO_QMAKE -query QT_INSTALL_PLUGINS`
SYS_TRANSLATIONS_DIR=`$PATH_TO_QMAKE -query QT_INSTALL_TRANSLATIONS`
SYS_FW_DIR=`$PATH_TO_QMAKE -query QT_INSTALL_LIBS`
SCRIPT_DIR=`pwd`/`dirname $0`


# Functions
function process_binary {
	local binary=$1
	local tab="$2"
	
	echo -e "$tab Processing" `basename $binary`
	
	if [ -f "$binary" ]; then
		#for str1 in `otool -LX "$binary" | grep $SYS_FW_DIR | cut -d " " -f 1`; do
		for str1 in `otool -LX "$binary" | grep Qt | cut -d " " -f 1`; do
			local str=`basename "$str1"`
			if [ `basename $binary` != $str ]; then
				local l="$str.framework/Versions/4/$str"
				echo -e "$tab\t Resolving dependency: $str"
				#install_name_tool -change "$SYS_FW_DIR/$l" "$DYLD_PREFIX/$l" "$binary"
				install_name_tool -change "$l" "$DYLD_PREFIX/$l" "$binary"
				process_lib "$l" "$tab\t"
			fi
		done;
		echo -e "$tab Processed" `basename $binary`
	else
		echo -e "$tab File not found: $binary"
	fi
}

function process_lib {
	local lib=$1
	local tab=$2
	
	if [ ! -f "$FW_DIR/$lib" ]; then			
		if [ -f "$SYS_FW_DIR/$lib" ]; then
			echo -e "$tab" `basename $lib` "still not in the bundle, fixing..."
			echo -en "$tab\t Copying " `basename "$lib"` " to bundle... " 
			cp -Rf $SYS_FW_DIR/`basename $lib`.framework $FW_DIR
			echo "copied!"
			install_name_tool -id "$DYLD_PREFIX/$lib" "$FW_DIR/$lib"
			strip -x "$FW_DIR/$lib"
			process_binary "$FW_DIR/$lib" "$tab\t"
			echo -e "$tab fixed" `basename $lib!`
		else
			echo "$tab ERROR: Library $lib not found"
		fi
	fi
}


# Creating temporary directories
cd $SCRIPT_DIR
rm -Rf $TMP_DIR
mkdir $TMP_DIR


echo "Building..."
cd $SVN_ROOT
$PATH_TO_QMAKE -recursive -spec macx-g++ $BUILDOPTS $ORIG_NAME.pro && make -j4 && make INSTALL_ROOT=$SCRIPT_DIR install
echo "Build done!"


# Copying info files
cd $SCRIPT_DIR
mkdir $TMP_DIR/doc
cp $SVN_ROOT/AUTHORS $TMP_DIR/doc/
cp $SVN_ROOT/CHANGELOG $TMP_DIR/doc/
cp $SVN_ROOT/README $TMP_DIR/doc/
cp $SVN_ROOT/COPYING $TMP_DIR/doc/


echo -n "Copying Qt plugins to bundle... "
cp -Rf "$SYS_PLUGINS_DIR/imageformats" "$CONTENTS_DIR/PlugIns"
echo "done!"

echo -n "Copying Qt locales to bundle... "
AVAIBLE_LANGUAGES=`ls $CONTENTS_DIR/Resources/translations/`
for lang in $AVAIBLE_LANGUAGES ; do
	cp "$SYS_TRANSLATIONS_DIR/qt_$lang.qm" "$CONTENTS_DIR/Resources/translations/$lang/qt_$lang.qm"
done
echo "done!"

strip "$CONTENTS_DIR/MacOS/vacuum"
process_binary "$CONTENTS_DIR/MacOS/vacuum" ""

for pl in `find "$CONTENTS_DIR" | egrep "\.dylib"` ; do
	strip -x "$pl"
	process_binary "$pl" ""
done


echo -n "Creating qt.conf... "
echo -e "[Paths]\nPlugins = PlugIns" > "$CONTENTS_DIR/Resources/qt.conf"
echo "done!"


echo -n "Cleaning up bundle... "
find "$CONTENTS_DIR/Frameworks" | egrep "debug|Headers" | xargs rm -rf
find "$CONTENTS_DIR/Frameworks" | egrep ".prl" | xargs rm
find "$CONTENTS_DIR" -type d -name .svn | xargs rm -rf
echo "done!"

echo -n "Set program version to Info.plist and copy InfoPlist.strings "
sed -i.bak "s/1.0.0.0/$VERSION/" $CONTENTS_DIR/Info.plist ; rm $CONTENTS_DIR/Info.plist.bak
mkdir $CONTENTS_DIR/Resources/en.lproj
cp $SVN_ROOT/src/packages/macosx/InfoPlist.strings $CONTENTS_DIR/Resources/en.lproj/InfoPlist.strings
echo "done!"

echo "Creating disk image..."

hdiutil create -ov -srcfolder $TMP_DIR -format UDBZ -volname "$PRODUCT_NAME" "$DMG_NAME.dmg"


echo -n "Cleaning up temp folder... "
rm -rf $TMP_DIR
echo "done!"
