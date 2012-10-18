#!/bin/bash
#
# make_dmg.sh - Add Qt libraries to Mac OS X bundle and pack it to .dmg image
# Copyright (C) 2010 Konstantin Tseplyaev (for Vacuum-IM project by Sergey Potapov)
# Copyright (C) 2012 Alexey Ivanov (for Vacuum-IM project by Sergey Potapov) 
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


### Version
REVISION="$(svnversion -c | cut -f 2 -d :)"
VER_NUMBER="$(grep 'CLIENT_VERSION ' src/definitions/version.h | cut -f 2 -d '"')"
VERSION="${VER_NUMBER}.${REVISION}"

### Namespace
ORIG_NAME="vacuum"
PRODUCT_NAME="Vacuum-IM"
DMG_NAME="${PRODUCT_NAME}_${VERSION}_macosx" 

### Environment
SVN_ROOT="../../.." 

# IMPORTANT! Path to qmake of Qt installation, against which project was built.
PATH_TO_QMAKE="qmake"

# Build options
BUILDOPTS=" -recursive -spec macx-g++ CONFIG+=release CONFIG+=x86_64"

# Directories
TMP_DIR="Applications"
DYLD_PREFIX="@executable_path/../Frameworks"
CONTENTS_DIR="$TMP_DIR/$ORIG_NAME.app/Contents"
FW_DIR="$CONTENTS_DIR/Frameworks"
SYS_PLUGINS_DIR=`$PATH_TO_QMAKE -query QT_INSTALL_PLUGINS`
SYS_TRANSLATIONS_DIR=`$PATH_TO_QMAKE -query QT_INSTALL_TRANSLATIONS`
SYS_FW_DIR=`$PATH_TO_QMAKE -query QT_INSTALL_LIBS`
SCRIPT_DIR=`pwd`/`dirname $0`


### Functions
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

### Building part

cd $SCRIPT_DIR
rm -Rf $TMP_DIR
if [ -d $TMP_DIR ]
then
    rm $TMP_DIR
fi
mkdir $TMP_DIR

echo -e "\033[7m Building... \033[0m"
cd $SVN_ROOT
$PATH_TO_QMAKE $BUILDOPTS $ORIG_NAME.pro && make -j4 && make INSTALL_ROOT=$SCRIPT_DIR install
echo "Build done!"

cd $SCRIPT_DIR
echo -e "\033[7m Copying Qt plugins to bundle... \033[0m"
cp -R "$SYS_PLUGINS_DIR/imageformats" "$CONTENTS_DIR/PlugIns"
echo "done!"

echo -e "\033[7m Copying Qt locales to bundle... \033[0m"
AVAIBLE_LANGUAGES=`ls $CONTENTS_DIR/Resources/translations/`
for lang in $AVAIBLE_LANGUAGES ; do
	cp "$SYS_TRANSLATIONS_DIR/qt_$lang.qm" "$CONTENTS_DIR/Resources/translations/$lang/qt_$lang.qm"
done
echo "done!"

echo -e "\033[7m Copying Qt libraries to bundle... \033[0m"
strip "$CONTENTS_DIR/MacOS/vacuum"
process_binary "$CONTENTS_DIR/MacOS/vacuum" ""
for pl in `find "$CONTENTS_DIR" | egrep "\.dylib"` ; do
	strip -x "$pl"
	process_binary "$pl" ""
done

echo -e "\033[7m Creating qt.conf... \033[0m"
echo -e "[Paths]\nPlugins = PlugIns" > "$CONTENTS_DIR/Resources/qt.conf"
echo "done!"

echo -e "\033[7m Cleaning up bundle... \033[0m"
find "$CONTENTS_DIR/Frameworks" | egrep "debug|Headers" | xargs rm -rf
find "$CONTENTS_DIR/Frameworks" | egrep ".prl" | xargs rm
find "$CONTENTS_DIR" -type d -name .svn | xargs rm -rf
echo "done!"

echo -e "\033[7m Setting program version to Info.plist and copy InfoPlist.strings... \033[0m"
sed -i.bak "s/1.0.0.0/$VERSION/" $CONTENTS_DIR/Info.plist ; rm $CONTENTS_DIR/Info.plist.bak
mkdir $CONTENTS_DIR/Resources/en.lproj
cp $SCRIPT_DIR/InfoPlist.strings $CONTENTS_DIR/Resources/en.lproj/InfoPlist.strings
echo "done!"

### DMG part

cd $SCRIPT_DIR
TMP_DMG_NAME="temporary.dmg"
if [ -f $TMP_DMG_NAME ]
then
	rm $TMP_DMG_NAME
fi

echo -e "\033[7m Creating temporary dmg disk image... \033[0m"
hdiutil create -ov -srcfolder $TMP_DIR -format UDRW -volname "$PRODUCT_NAME" "$TMP_DMG_NAME"

echo -e "\033[7m Mounting temporary image... \033[0m"
for i in /Volumes/${PRODUCT_NAME}*
do
	if [[ -d $i ]]
	then
		hdiutil unmount "$i"
	fi
done
device=$(hdiutil attach -readwrite -noverify -noautoopen "$TMP_DMG_NAME" | egrep '^/dev/' | sed 1q | awk '{print $1}')
echo "done! (device ${device})"

echo -e "\033[7m Sleeping for 5 seconds... \033[0m"
sleep 5

echo -e "\033[7m Setting style for temporary dmg image... \033[0m"
echo "* Copying background image... "
BG_FOLDER="/Volumes/$PRODUCT_NAME/.background"
mkdir "$BG_FOLDER"
cp "$SCRIPT_DIR/background.png" "${BG_FOLDER}/"
echo "* Copying volume icon... "
ICON_FOLDER="/Volumes/$PRODUCT_NAME"
cp "$SVN_ROOT/vacuum.icns" "$ICON_FOLDER/.VolumeIcon.icns"
echo "* Setting volume icon... "
SetFile -c icnC "$ICON_FOLDER/.VolumeIcon.icns"
SetFile -a C "$ICON_FOLDER"
echo "* Adding symlink to /Applications... "
ln -s /Applications "/Volumes/$PRODUCT_NAME/Applications"
echo "done!"

APPS_X=380
APPS_Y=185
BUNDLE_X=110
BUNDLE_Y=185
WINDOW_WIDTH=500
WINDOW_HEIGHT=300
WINDOW_LEFT=300
WINDOW_TOP=100
WINDOW_RIGHT=$(($WINDOW_LEFT + $WINDOW_WIDTH))
WINDOW_BOTTOM=$(($WINDOW_TOP + $WINDOW_HEIGHT))

echo -e "\033[7m Executing applescript for further customization... \033[0m"
APPLESCRIPT="
tell application \"Finder\"
	tell disk \"$PRODUCT_NAME\"
		open
		-- Setting view options
		set current view of container window to icon view
		set toolbar visible of container window to false
		set statusbar visible of container window to false
		set the bounds of container window to {${WINDOW_LEFT}, ${WINDOW_TOP}, ${WINDOW_RIGHT}, ${WINDOW_BOTTOM}}
		set theViewOptions to the icon view options of container window
		set arrangement of theViewOptions to not arranged
		set icon size of theViewOptions to 96
		-- Settings background
		${NO_BG}set background picture of theViewOptions to file \".background:background.png\"
		-- Rearranging
		set the position of item \"Applications\" to {$APPS_X, $APPS_Y}
		set the position of item \"$PRODUCT_NAME\" to {$BUNDLE_X, $BUNDLE_Y}
		-- Updating and sleeping for 5 secs
		update without registering applications
		-- Reopening
		close
		open
		delay 6
		-- Reopening, for .DS_STORE
		close
		open
		delay 6
		-- Eject
		eject
	end tell
end tell
"
echo "$APPLESCRIPT" | osascript
echo "done!"

echo -e "\033[7m Fixing permissins and syncing... \033[0m"
chmod -Rf go-w /Volumes/"$PRODUCT_NAME" &> /dev/null || true
sync
echo "done!"

echo -e "\033[7m Converting... \033[0m"
if [ -f $DMG_NAME ]
then
	rm $DMG_NAME
fi
hdiutil convert "$TMP_DMG_NAME" -format UDBZ -o "$SCRIPT_DIR/$DMG_NAME"
echo "done!"

echo -e "\033[7m Removing temporary folder... \033[0m"
rm $TMP_DMG_NAME
rm -rf $TMP_DIR
echo "done!"

echo -e "\033[7m Everything done. DMG disk image is ready for distribution. \033[0m"
