include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")

if (WIN32)
	foreach(LANG ${LOCALIZED_LANGS})
		string(SUBSTRING ${LANG} 0 2 SHORTLANG)
		if(EXISTS "${_qt5Core_install_prefix}/translations/qtbase_${SHORTLANG}.qm")
			install(FILES "${_qt5Core_install_prefix}/translations/qtbase_${SHORTLANG}.qm"
					DESTINATION "${CMAKE_INSTALL_PREFIX}/${INSTALL_TRANSLATIONS}/${LANG}" RENAME "qt_${SHORTLANG}.qm" COMPONENT Runtime)
		else()
			install(FILES "${_qt5Core_install_prefix}/translations/qt_${SHORTLANG}.qm"
					DESTINATION "${CMAKE_INSTALL_PREFIX}/${INSTALL_TRANSLATIONS}/${LANG}" COMPONENT Runtime)
		endif()
	endforeach(LANG)
	
	set(_additional_binaries "${_qt5Core_install_prefix}/bin/libEGL.dll")
	set(_additional_binaries "${_qt5Core_install_prefix}/bin/libGLESv2.dll")
	set(_additional_binaries "${_qt5Core_install_prefix}/bin/d3dcompiler_47.dll")
	install(FILES "${_additional_binaries}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}" COMPONENT Runtime)

	list(APPEND QtBinaries ${Qt5Gui_PLUGINS})
	list(APPEND QtBinaries Qt5::QWindowsAudioPlugin)
	list(APPEND QtBinaries Qt5::QWindowsPrinterSupportPlugin)
	list(APPEND QtBinaries Qt5::QGenericEnginePlugin)
	list(APPEND QtBinaries Qt5::QSQLiteDriverPlugin)
	list(APPEND QtBinaries Qt5::QSvgPlugin)
	list(APPEND QtBinaries Qt5::QSvgIconPlugin)

	foreach(_qtbinary ${QtBinaries})
		get_target_property(_qtbinary_fullpath ${_qtbinary} LOCATION)
		get_filename_component(_qtbinary_dirpath ${_qtbinary_fullpath} PATH)
		get_filename_component(_qtbinary_dirname ${_qtbinary_dirpath} NAME)
		get_filename_component(_qtbinary_file ${_qtbinary_fullpath} NAME)
		if(${_qtbinary_dirname} MATCHES "bin")
			set(_qtbinary_dirname "")
		endif()
		list(APPEND DLL_FILES "${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}/${_qtbinary_dirname}/${_qtbinary_file}")
		install(FILES ${_qtbinary_fullpath} DESTINATION "${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}/${_qtbinary_dirname}" COMPONENT Runtime)
	endforeach()

	get_property(_plugins GLOBAL PROPERTY ALL_PLUGINS_FULLPATHS)
	list(APPEND DLL_FILES "${_plugins}")

	list(APPEND DLL_DIRS ${_qt5Core_install_prefix}/bin)
	list(APPEND DLL_DIRS ${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR})
	list(APPEND DLL_DIRS ${CMAKE_INSTALL_PREFIX}/${INSTALL_PLUGINS})

	set(CMAKE_INSTALL_UCRT_LIBRARIES ON)
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${INSTALL_APP_DIR}")
	include(InstallRequiredSystemLibraries)

	install(CODE "
		set(BU_CHMOD_BUNDLE_ITEMS ON)
		include(BundleUtilities)
		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${INSTALL_APP_DIR}/vacuum.exe\" \"${DLL_FILES}\" \"${DLL_DIRS}\")" COMPONENT Runtime)

endif (WIN32)
