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

	function(copyQM _qm)
		if(EXISTS ${_qm})
			file(COPY "${_qm}" DESTINATION "${CMAKE_BINARY_DIR}/${INSTALL_TRANSLATIONS}/${LANG}")
		endif()
	endfunction()

	foreach(LANG ${LOCALIZED_LANGS})
		string(SUBSTRING ${LANG} 0 2 SHORTLANG)
		copyQM("${_qt5Core_install_prefix}/translations/qt_${SHORTLANG}.qm")
		copyQM("${_qt5Core_install_prefix}/translations/qtbase_${SHORTLANG}.qm")
		copyQM("${_qt5Core_install_prefix}/translations/qtscript_${SHORTLANG}.qm")
		copyQM("${_qt5Core_install_prefix}/translations/qtquick1_${SHORTLANG}.qm")
		copyQM("${_qt5Core_install_prefix}/translations/qtmultimedia_${SHORTLANG}.qm")
		copyQM("${_qt5Core_install_prefix}/translations/qtxmlpatterns_${SHORTLANG}.qm")
	endforeach(LANG)

	list(APPEND MODULES_LIST ${Qt5Gui_PLUGINS})
	list(APPEND MODULES_LIST Qt5::CoreAudioPlugin)
	list(APPEND MODULES_LIST Qt5::QCocoaPrinterSupportPlugin)
	list(APPEND MODULES_LIST Qt5::QGenericEnginePlugin)
	list(APPEND MODULES_LIST Qt5::QSQLiteDriverPlugin)
	list(APPEND MODULES_LIST Qt5::QSvgPlugin)
	list(APPEND MODULES_LIST Qt5::QSvgIconPlugin)

	foreach(plugin ${MODULES_LIST})
		get_target_property(_loc ${plugin} LOCATION)
		get_filename_component(_plugindir ${_loc} PATH)
		get_filename_component(_pluginname ${_plugindir} NAME)
		get_filename_component(_pluginfile ${_loc} NAME)
		list(APPEND DYLIB_FILES "${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}/Contents/PlugIns/${_pluginname}/${_pluginfile}")
		install(CODE "
			file(COPY ${_loc} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}/Contents/PlugIns/${_pluginname}\")" COMPONENT Runtime)
	endforeach()
	
	get_property(_plugins GLOBAL PROPERTY ALL_PLUGINS_DYLIB_FILES)
	list(APPEND DYLIB_FILES "${_plugins}")

	list(APPEND FRAMEWORKS_DIRS ${_qt5Core_install_prefix}/lib)
	list(APPEND FRAMEWORKS_DIRS ${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}/Contents/Frameworks)
	list(APPEND FRAMEWORKS_DIRS ${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}/Contents/PlugIns)

	install(CODE "
		set(BU_CHMOD_BUNDLE_ITEMS ON)
		include(BundleUtilities)
		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}\" \"${DYLIB_FILES}\" \"${FRAMEWORKS_DIRS}\")" COMPONENT Runtime)
endif (APPLE)
