; FL Studio Rich Presence Installer
; Modern UI and enhanced functionality

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
; Installer Information
Name "FL Studio Rich Presence"
OutFile "FLRP_Installer.exe"
InstallDir "$PROGRAMFILES\FL Studio Rich Presence"
InstallDirRegKey HKLM "Software\FL Studio Rich Presence" "InstallDir"

; Request administrator privileges
RequestExecutionLevel admin

; Modern UI Configuration
!define MUI_ABORTWARNING
!define MUI_ICON "public\FLRP.ico"
!define MUI_UNICON "public\FLRP.ico"
!define MUI_BGCOLOR "5865f2"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
Page custom FLStudioPathPage
!insertmacro MUI_PAGE_INSTFILES
Page custom FinalInstructionsPage
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; Variables
Var FLStudioPath
Var FLScriptPath
Var FLStudioFound
Var VirtualMIDIFound

; Custom page for FL Studio path
Function FLStudioPathPage
    ; Try to detect FL Studio installation
    Call DetectFLStudio
    
    ${If} $FLStudioFound == "0"
        MessageBox MB_ICONQUESTION|MB_YESNO "FL Studio installation not detected automatically.$\nWould you like to manually specify the FL Studio installation directory?" IDYES manual IDNO skip
        manual:
            nsDialogs::SelectFolderDialog "Select FL Studio installation directory (contains FL64.exe)" "$PROGRAMFILES"
            Pop $0
            ${If} $0 != ""
                ${If} ${FileExists} "$0\FL64.exe"
                    StrCpy $FLStudioPath "$0"
                    StrCpy $FLStudioFound "1"
                    StrCpy $FLScriptPath "$PROFILE\Documents\Image-Line\FL Studio\Settings\Hardware"
                    CreateDirectory $FLScriptPath
                    MessageBox MB_ICONINFORMATION "FL Studio found at: $FLStudioPath"
                ${Else}
                    MessageBox MB_ICONEXCLAMATION "FL64.exe not found in selected directory.$\nFL Studio integration will be skipped."
                ${EndIf}
            ${EndIf}
        skip:
    ${EndIf}
    
    ; Check for virtual MIDI ports
    Call CheckVirtualMIDI
FunctionEnd

; Function to detect FL Studio installation
Function DetectFLStudio
    StrCpy $FLStudioFound "0"
    
    ; Check FL Studio path from shared registry (works for all versions sonetimes)
    ReadRegStr $0 HKLM "SOFTWARE\Image-Line\Shared\Paths" "Install path"
    ${If} $0 != ""
        StrCpy $FLStudioPath $0
        StrCpy $FLStudioFound "1"
        Goto found
    ${EndIf}
    
    ; Current Version FL Studio
    ReadRegStr $0 HKLM "SOFTWARE\Image-Line\Registrations\FL Studio 25.1" ""
    ${If} $0 != ""
        ; Registry key exists, check default path
        ${If} ${FileExists} "$PROGRAMFILES\Image-Line\FL Studio 2025\FL64.exe"
            StrCpy $FLStudioPath "$PROGRAMFILES\Image-Line\FL Studio 2025"
            StrCpy $FLStudioFound "1"
            Goto found
        ${EndIf}
    ${EndIf}
    
    ; Legacy checks for older versions
    ReadRegStr $0 HKLM "SOFTWARE\Image-Line\FL Studio 21" "InstallDir"
    ${If} $0 != ""
        StrCpy $FLStudioPath $0
        StrCpy $FLStudioFound "1"
        Goto found
    ${EndIf}
    
    ReadRegStr $0 HKLM "SOFTWARE\Image-Line\FL Studio 20" "InstallDir"
    ${If} $0 != ""
        StrCpy $FLStudioPath $0
        StrCpy $FLStudioFound "1"
        Goto found
    ${EndIf}
    
    ; Check default installation paths
    ${If} ${FileExists} "$PROGRAMFILES\Image-Line\FL Studio 2025\FL64.exe"
        StrCpy $FLStudioPath "$PROGRAMFILES\Image-Line\FL Studio 2025"
        StrCpy $FLStudioFound "1"
        Goto found
    ${EndIf}
    
    ${If} ${FileExists} "$PROGRAMFILES\Image-Line\FL Studio 21\FL64.exe"
        StrCpy $FLStudioPath "$PROGRAMFILES\Image-Line\FL Studio 21"
        StrCpy $FLStudioFound "1"
        Goto found
    ${EndIf}
    
    ${If} ${FileExists} "$PROGRAMFILES\Image-Line\FL Studio 20\FL64.exe"
        StrCpy $FLStudioPath "$PROGRAMFILES\Image-Line\FL Studio 20"
        StrCpy $FLStudioFound "1"
        Goto found
    ${EndIf}
    
    found:
    ${If} $FLStudioFound == "1"
        ; Construct script path with FLRP subfolder
        StrCpy $FLScriptPath "$PROFILE\Documents\Image-Line\FL Studio\Settings\Hardware\FLRP"
        
        ; Create directory if it doesn't exist
        CreateDirectory $FLScriptPath
    ${EndIf}
FunctionEnd

; Function to check for virtual MIDI ports
Function CheckVirtualMIDI
    StrCpy $VirtualMIDIFound "0"
    
    ; Check for loopMIDI ports in registry
    ReadRegStr $0 HKLM "SOFTWARE\Tobias Erichsen\loopMIDI" ""
    ${If} $0 != ""
        StrCpy $VirtualMIDIFound "1"
        DetailPrint "loopMIDI found in registry"
    ${EndIf}
    
    ; Check for other virtual MIDI drivers
    ${If} $VirtualMIDIFound == "0"
        ; Check for Windows built-in MIDI loopback
        ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Services\midiloop" ""
        ${If} $0 != ""
            StrCpy $VirtualMIDIFound "1"
            DetailPrint "Windows MIDI loopback found"
        ${EndIf}
    ${EndIf}
    
    ${If} $VirtualMIDIFound == "0"
        MessageBox MB_ICONINFORMATION "Virtual MIDI Port Required$\n$\nFor FL Studio integration to work, you need a virtual MIDI port.$\n$\nRecommended: Download and install loopMIDI from tobias-erichsen.de$\n$\nAfter installation:$\n1. Open loopMIDI$\n2. Create a port named 'FLRP' or similar$\n3. Configure FL Studio to use this MIDI port$\n$\nThe installer will continue, but FL Studio integration may not work without this."
    ${EndIf}
FunctionEnd

; Final configuration instructions page
Function FinalInstructionsPage
    ${If} $FLStudioFound == "1"
        MessageBox MB_ICONINFORMATION "FL Studio Configuration Required$\n$\nTo complete setup, please configure FL Studio:$\n$\n1. Open FL Studio$\n2. Go to Options > MIDI Settings$\n3. Find your virtual MIDI port (e.g., 'FLRP')$\n4. Set the SAME NUMBER for both Input and Output$\n5. Set Controller Type to 'FLRP Script'$\n6. Turn OFF 'Send Master Sync' for this port$\n7. Click Apply$\n$\nThis ensures proper communication between FL Studio and the Rich Presence application."
    ${EndIf}
FunctionEnd

; Installation Sections
Section "Main Application" SecMain
    SectionIn RO ; Required section
    
    SetOutPath "$INSTDIR"
    
    ; Install main executable
    File "build\Release\FLRP.exe"
    File "public\FLRP.ico"
    
    ; Create .env file with state file path (points to where Python script creates JSON)
    ${If} $FLStudioFound == "1"
        FileOpen $1 "$INSTDIR\.env" w
        FileWrite $1 "STATE_FILE_PATH=$FLScriptPath\fl_studio_state.json$\r$\n"
        FileWrite $1 "DISCORD_CLIENT_ID=1234567890123456789$\r$\n"
        FileWrite $1 "DEBUG_MODE=false$\r$\n"
        FileWrite $1 "POLL_INTERVAL_MS=1000$\r$\n"
        FileClose $1
        DetailPrint "Created .env file with state file path: $FLScriptPath\fl_studio_state.json"
    ${Else}
        ; Create .env file with default state file path if FL Studio not found
        FileOpen $1 "$INSTDIR\.env" w
        FileWrite $1 "STATE_FILE_PATH=$PROFILE\Documents\Image-Line\FL Studio\Settings\Hardware\FLRP\fl_studio_state.json$\r$\n"
        FileWrite $1 "DISCORD_CLIENT_ID=1234567890123456789$\r$\n"
        FileWrite $1 "DEBUG_MODE=false$\r$\n"
        FileWrite $1 "POLL_INTERVAL_MS=1000$\r$\n"
        FileClose $1
        DetailPrint "Created .env file with default state file path"
    ${EndIf}
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    
    ; Registry entries
    WriteRegStr HKLM "Software\FL Studio Rich Presence" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FL Studio Rich Presence" "DisplayName" "FL Studio Rich Presence"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FL Studio Rich Presence" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FL Studio Rich Presence" "DisplayIcon" "$INSTDIR\FLRP.ico"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FL Studio Rich Presence" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FL Studio Rich Presence" "NoRepair" 1
    
SectionEnd

Section "FL Studio Integration" SecFLStudio
    ${If} $FLStudioFound == "1"
        SetOutPath "$FLScriptPath"
        File "python\device_FLRP.py"
        
        DetailPrint "FL Studio script installed to: $FLScriptPath"
        
        ${If} $VirtualMIDIFound == "0"
            DetailPrint "Warning: No virtual MIDI port detected"
            MessageBox MB_ICONEXCLAMATION "FL Studio script installed, but no virtual MIDI port was detected.$\n$\nFor the integration to work:$\n1. Install loopMIDI (recommended)$\n2. Create a virtual MIDI port$\n3. Configure FL Studio to use the MIDI port"
        ${EndIf}
    ${Else}
        DetailPrint "FL Studio not detected - script installation skipped"
        MessageBox MB_ICONINFORMATION "FL Studio integration skipped.$\nTo install manually, copy device_FLRP.py to your FL Studio Hardware settings folder."
    ${EndIf}
SectionEnd

Section "Desktop Shortcut" SecDesktop
    CreateShortCut "$DESKTOP\FL Studio Rich Presence.lnk" "$INSTDIR\FLRP.exe" "" "$INSTDIR\FLRP.ico"
SectionEnd

Section "Start Menu Shortcut" SecStartMenu
    CreateDirectory "$SMPROGRAMS\FL Studio Rich Presence"
    CreateShortCut "$SMPROGRAMS\FL Studio Rich Presence\FL Studio Rich Presence.lnk" "$INSTDIR\FLRP.exe" "" "$INSTDIR\FLRP.ico"
    CreateShortCut "$SMPROGRAMS\FL Studio Rich Presence\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

; Section Descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} "Core application files (required)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecFLStudio} "Installs FL Studio integration script"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} "Creates desktop shortcut"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} "Creates Start Menu shortcuts"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Uninstaller
Section "Uninstall"
    ; Remove files
    Delete "$INSTDIR\FLRP.exe"
    Delete "$INSTDIR\FLRP.ico"
    Delete "$INSTDIR\.env"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"
    
    ; Remove FL Studio script and folder
    Delete "$PROFILE\Documents\Image-Line\FL Studio\Settings\Hardware\FLRP\device_FLRP.py"
    Delete "$PROFILE\Documents\Image-Line\FL Studio\Settings\Hardware\FLRP\fl_studio_state.json"
    RMDir "$PROFILE\Documents\Image-Line\FL Studio\Settings\Hardware\FLRP"
    
    ; Remove shortcuts
    Delete "$DESKTOP\FL Studio Rich Presence.lnk"
    Delete "$SMPROGRAMS\FL Studio Rich Presence\FL Studio Rich Presence.lnk"
    Delete "$SMPROGRAMS\FL Studio Rich Presence\Uninstall.lnk"
    RMDir "$SMPROGRAMS\FL Studio Rich Presence"
    
    ; Remove registry entries
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FL Studio Rich Presence"
    DeleteRegKey HKLM "Software\FL Studio Rich Presence"
SectionEnd