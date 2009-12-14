; vacuum-im.nsi
;
; This script creates vacuum-im installer for windows
;--------------------------------

; Install Folder
!define PROGRAM_FOLDER      "Vacuum IM"

; Install Start Menu Folder
!define PROGRAM_SM_FOLDER   "Vacuum IM"

; Registry Key
!define PROGRAM_REG_KEY     "VacuumIM"

; Program binaries
!define PROGRAM_BIN_FOLDER  "..\..\.."

; Qt folder
!define QT_BIN_FOLDER       "$%QTDIR%"

; The name of the installer
Name "Vacuum IM"

; The file to write
OutFile "vacuum_setup.exe"

; License file name
LicenseData "${PROGRAM_BIN_FOLDER}\COPYING"

; The default installation directory
InstallDir "$PROGRAMFILES\${PROGRAM_FOLDER}"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\${PROGRAM_REG_KEY}" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin


;--------------------------------
; Pages
;--------------------------------
Page license
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles


;--------------------------------
; The stuff to install
;--------------------------------
Section "VacuumIM (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Info files
  File "${PROGRAM_BIN_FOLDER}\README"
	
  ; Binaries
  File "${PROGRAM_BIN_FOLDER}\vacuum.exe"
  File "${PROGRAM_BIN_FOLDER}\vacuumutils.dll"

  ; OpenSSL libraries
  File "${PROGRAM_BIN_FOLDER}\libeay32.dll"
  File "${PROGRAM_BIN_FOLDER}\libssl32.dll"
	
	; Visual Studio redistribute files
  File "${PROGRAM_BIN_FOLDER}\msvcr90.dll"
	
	; MinGW redistribute files
  ;File "${PROGRAM_BIN_FOLDER}\libgcc_s_dw2-1.dll"
  ;File "${PROGRAM_BIN_FOLDER}\mingwm10.dll"

  ; Qt modules
  File "${QT_BIN_FOLDER}\bin\QtCore4.dll"
  File "${QT_BIN_FOLDER}\bin\QtGui4.dll"
  File "${QT_BIN_FOLDER}\bin\QtNetwork4.dll"
  File "${QT_BIN_FOLDER}\bin\QtXml4.dll"
  File "${QT_BIN_FOLDER}\bin\QtWebkit4.dll"

	; Qt plugins
	SetOutPath $INSTDIR\imageformats
  File "${QT_BIN_FOLDER}\plugins\imageformats\qgif4.dll"
  File "${QT_BIN_FOLDER}\plugins\imageformats\qico4.dll"
  File "${QT_BIN_FOLDER}\plugins\imageformats\qjpeg4.dll"
  File "${QT_BIN_FOLDER}\plugins\imageformats\qmng4.dll"
  File "${QT_BIN_FOLDER}\plugins\imageformats\qsvg4.dll"
  File "${QT_BIN_FOLDER}\plugins\imageformats\qtiff4.dll"

  ; Plugins
	SetOutPath $INSTDIR\plugins
  File /r /x .svn "${PROGRAM_BIN_FOLDER}\plugins\*.*"
	
	; Resources
	SetOutPath $INSTDIR\resources
  File /r /x .svn "${PROGRAM_BIN_FOLDER}\resources\*.*"
	
	; Translations
	SetOutPath $INSTDIR\translations
  File /r /x .svn "${PROGRAM_BIN_FOLDER}\translations\*.*"
	
  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\${PROGRAM_REG_KEY}" "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_REG_KEY}" "DisplayName" "Vacuum Instant Messenger"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_REG_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_REG_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_REG_KEY}" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\${PROGRAM_SM_FOLDER}"
  CreateShortCut "$SMPROGRAMS\${PROGRAM_SM_FOLDER}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${PROGRAM_SM_FOLDER}\Vacuum IM.lnk" "$INSTDIR\vacuum.exe" "" "$INSTDIR\vacuum.exe" 0
  
SectionEnd

Section "Desktop Shortcuts"

  CreateShortCut "$DESKTOP\Vacuum IM.lnk" "$INSTDIR\vacuum.exe" "" "$INSTDIR\vacuum.exe" 0
  
SectionEnd


;--------------------------------
; Uninstaller
;--------------------------------
Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_REG_KEY}"
  DeleteRegKey HKLM "SOFTWARE\${PROGRAM_REG_KEY}"

  ; Remove desktop icon
  Delete "$DESKTOP\Vacuum IM.lnk"
	
  ; Remove directories used
  RMDir /r "$SMPROGRAMS\${PROGRAM_SM_FOLDER}"
  RMDir /r "$INSTDIR"

SectionEnd
