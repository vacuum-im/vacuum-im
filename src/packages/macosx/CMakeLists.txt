include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")

if (APPLE)
	set_target_properties(${VACUUM_LOADER_NAME} PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/src/packages/macosx/MacOSXBundleInfo.plist.in"
		MACOSX_BUNDLE_BUNDLE_NAME "vacuum"
		MACOSX_BUNDLE_BUNDLE_DISPLAY_NAME "vacuum"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}"
		MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION}"
		MACOSX_BUNDLE_ICON_FILE "vacuum.icns"
		MACOSX_BUNDLE_GUI_IDENTIFIER "org.vacuum-im"
		MACOSX_BUNDLE_COPYRIGHT "Copyright 2010-2017, Potapov S.A.")

	file(COPY "${CMAKE_SOURCE_DIR}/vacuum.icns" DESTINATION "${CMAKE_BINARY_DIR}/${INSTALL_RESOURCES}")
	file(WRITE "${CMAKE_BINARY_DIR}/${INSTALL_RESOURCES}/qt.conf" "[Paths]\nPlugins = PlugIns")
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_RESOURCES}/en.lproj")
	file(COPY "${CMAKE_SOURCE_DIR}/src/packages/macosx/InfoPlist.strings" DESTINATION "${CMAKE_BINARY_DIR}/${INSTALL_RESOURCES}/en.lproj/")

	foreach(LANG ${LOCALIZED_LANGS})
		string(SUBSTRING ${LANG} 0 2 SHORTLANG)
		if(EXISTS "${_qt5Core_install_prefix}/translations/qtbase_${SHORTLANG}.qm")
			file(COPY "${_qt5Core_install_prefix}/translations/qtbase_${SHORTLANG}.qm"
					DESTINATION "${CMAKE_BINARY_DIR}/${INSTALL_TRANSLATIONS}/${LANG}")
			file(RENAME "${CMAKE_BINARY_DIR}/${INSTALL_TRANSLATIONS}/${LANG}/qtbase_${SHORTLANG}.qm"
						"${CMAKE_BINARY_DIR}/${INSTALL_TRANSLATIONS}/${LANG}/qt_${SHORTLANG}.qm" )
		else()
			file(COPY "${_qt5Core_install_prefix}/translations/qt_${SHORTLANG}.qm"
					DESTINATION "${CMAKE_BINARY_DIR}/${INSTALL_TRANSLATIONS}/${LANG}")
		endif()
	endforeach(LANG)

	list(APPEND QtBinaries ${Qt5Gui_PLUGINS})
	list(APPEND QtBinaries Qt5::CoreAudioPlugin)
	list(APPEND QtBinaries Qt5::QCocoaPrinterSupportPlugin)
	list(APPEND QtBinaries Qt5::QGenericEnginePlugin)
	list(APPEND QtBinaries Qt5::QSQLiteDriverPlugin)
	list(APPEND QtBinaries Qt5::QSvgPlugin)
	list(APPEND QtBinaries Qt5::QSvgIconPlugin)

	foreach(_qtbinary ${QtBinaries})
		get_target_property(_qtbinary_fullpath ${_qtbinary} LOCATION)
		get_filename_component(_qtbinary_dirpath ${_qtbinary_fullpath} PATH)
		get_filename_component(_qtbinary_dirname ${_qtbinary_dirpath} NAME)
		get_filename_component(_qtbinary_file ${_qtbinary_fullpath} NAME)
		list(APPEND DYLIB_FILES "${CMAKE_INSTALL_PREFIX}/${INSTALL_PLUGINS}/${_qtbinary_dirname}/${_qtbinary_file}")
		install(CODE "
			file(COPY ${_qtbinary_fullpath} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${INSTALL_PLUGINS}/${_qtbinary_dirname}\")" COMPONENT Runtime)
	endforeach()
	
	get_property(_plugins GLOBAL PROPERTY ALL_PLUGINS_FULLPATHS)
	list(APPEND DYLIB_FILES "${_plugins}")

	list(APPEND DYLIB_DIRS ${_qt5Core_install_prefix}/lib)
	list(APPEND DYLIB_DIRS ${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}/Contents/Frameworks)
	list(APPEND DYLIB_DIRS ${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}/Contents/PlugIns)

	install(CODE "
		set(BU_CHMOD_BUNDLE_ITEMS ON)
		include(BundleUtilities)
		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}\" \"${DYLIB_FILES}\" \"${DYLIB_DIRS}\")" COMPONENT Runtime)
endif (APPLE)
