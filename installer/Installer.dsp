# Microsoft Developer Studio Project File - Name="Installer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Installer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Installer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Installer.mak" CFG="Installer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Installer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Installer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Installer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_INSTALLER_" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /machine:I386 /out:"../../_bin/Admin_Lancher.exe"

!ELSEIF  "$(CFG)" == "Installer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_INSTALLER_" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../../_bin/Admin_Lancher.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Installer - Win32 Release"
# Name "Installer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AmmyyInstApp.cpp
# End Source File
# Begin Source File

SOURCE=.\AmmyyInstApp.h
# End Source File
# Begin Source File

SOURCE=..\main\Common.cpp
# End Source File
# Begin Source File

SOURCE=..\main\Common.h
# End Source File
# Begin Source File

SOURCE=.\DlgMain.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgMain.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "RL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\RL\RLBase64Coder.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLBase64Coder.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLDlgTemplate.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLDlgTemplate.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLEH.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLEH.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLException.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLException.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLHttp.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLHttp.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLLock.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLLock.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLLog.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLLog.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLStream.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLStream.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLTimer.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLTimer.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLWnd.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\StringA.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\StringA.h
# End Source File
# End Group
# Begin Group "unzip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\unzip\LiteUnzip.cpp
# End Source File
# Begin Source File

SOURCE=..\common\unzip\LiteUnzip.h
# End Source File
# Begin Source File

SOURCE=..\common\unzip\resource.h
# End Source File
# End Group
# End Target
# End Project
