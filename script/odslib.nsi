# Copyright 2023 Ingemar Hedvall
# SPDX-License-Identifier: MIT 

!include MUI2.nsh
!include x64.nsh
!include FileFunc.nsh
 
Name "ODS Library Applications 1.0"
OutFile "..\cmake-build-release\odslib_setup.exe"
Unicode True

RequestExecutionLevel admin

Var StartMenuFolder

InstallDir "$LOCALAPPDATA\odslib"
InstallDirRegKey HKLM "Software\OdsLib" ""


!define MUI_ABORTWARNING
!define ARP "Software\Microsoft\Windows\CurrentVersion\Uninstall\OdsLib"
!define MSVS_DIR "d:\msvs"
!define APP_BUILD_DIR "..\cmake-build-release"
!define UTIL_BUILD_DIR "..\cmake-build-release\_deps\utillib-build"
!define MODEL_SOURCE_DIR "..\odsconfigurator\model"


!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY

!define MUI_STARTMENUPAGE_DEFAULTFOLDER "ODS Library Applications"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\OdsLib"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
   
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
  
!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections
Section "Visual Studio Runtime" MSCRT
  SectionIn RO
  SetRegView 64
  SetOutPath "$INSTDIR\bin"
  File "${MSVS_DIR}\VC_redist.x64.exe"
  ExecWait '"$INSTDIR\bin\VC_redist.x64.exe" /passive /norestart'
  Delete "$INSTDIR\bin\VC_redist.x64.exe.exe"
SectionEnd

Section "Applications" APP
  SectionIn RO
  SetRegView 64	
  SetShellVarContext all

  SetOutPath "$INSTDIR\bin"
  File "${UTIL_BUILD_DIR}\listend\*.exe"
  File "${UTIL_BUILD_DIR}\listenviewer\*.exe"
  File "${UTIL_BUILD_DIR}\serviced\serviced.exe"
  File "${UTIL_BUILD_DIR}\serviceexplorer\serviceexplorer.exe"
  
  File "${APP_BUILD_DIR}\odsconfigurator\odsconfig.exe"
  File "${APP_BUILD_DIR}\eventlogrpcserverd\eventlogrpcd.exe"
  File "${APP_BUILD_DIR}\reportexplorer\reportexplorer.exe"
  
  SetOutPath "$INSTDIR\img"
  File "..\img\*.ico"
  
  SetOutPath "$APPDATA\eventlog\model"
  File "${MODEL_SOURCE_DIR}\eventlogdb.xml"
  
  ;Store installation folder
  WriteRegStr HKLM "Software\OdsLib" "" $INSTDIR

  WriteRegNone HKLM "${ARP}" "" 
  WriteRegStr HKLM "${ARP}" "InstallLocation" $INSTDIR
  WriteRegStr HKLM "${ARP}" "DisplayIcon" "$INSTDIR\img\odslib.ico"
  WriteRegStr HKLM "${ARP}" "DisplayName" "ODS Library Applications 1.0"
  WriteRegStr HKLM "${ARP}" "DisplayVersion" "1.0.0"
  WriteRegStr HKLM "${ARP}" "Publisher" "Ingemar Hedvall" 
  WriteRegDWORD HKLM "${ARP}" "NoModify" 1 
  WriteRegDWORD HKLM "${ARP}" "NoRepair" 1 
  WriteRegDWORD HKLM "${ARP}" "VersionMajor" 1 
  WriteRegDWORD HKLM "${ARP}" "VersionMinor" 0 	  
			 
  WriteRegStr HKLM "${ARP}"  "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Listen Viewer.lnk" "$INSTDIR\bin\listenviewer.exe"
  CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Service Explorer.lnk" "$INSTDIR\bin\serviceexplorer.exe"
  CreateShortcut "$SMPROGRAMS\$StartMenuFolder\ODS Configurator.lnk" "$INSTDIR\bin\odsconfig.exe"
  CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Report Explorer.lnk" "$INSTDIR\bin\reportexplorer.exe"
  !insertmacro MUI_STARTMENU_WRITE_END 
  
  ; Create Uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Add /Remove Programs
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "${ARP}" "EstimatedSize" "$0"
SectionEnd

LangString DESC_CRT ${LANG_ENGLISH} "Microsoft Visual Studio Runtime."
LangString DESC_APP ${LANG_ENGLISH} "All ODS Library Executables."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${MSCRT} $(DESC_CRT)
  !insertmacro MUI_DESCRIPTION_TEXT ${APP} $(DESC_APP)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"
  SetRegView 64	  
  SetShellVarContext all
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  
  Delete "$INSTDIR\Uninstall.exe"

  RMDir /r "$INSTDIR"  
  RMDir /r "$SMPROGRAMS\$StartMenuFolder"
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OdsLib"
  DeleteRegKey HKLM "Software\OdsLib"

SectionEnd