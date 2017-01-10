# Microsoft Developer Studio Project File - Name="Customizer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Customizer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Customizer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Customizer.mak" CFG="Customizer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Customizer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Customizer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Customizer - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 Gdi32.lib User32.lib /nologo /subsystem:windows /machine:I386 /out:"../../_bin/Ammyy_Customizer.exe"

!ELSEIF  "$(CFG)" == "Customizer - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /out:"../../_bin/Ammyy_Customizer.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Customizer - Win32 Release"
# Name "Customizer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AmmyCustomizer.cpp
# End Source File
# Begin Source File

SOURCE=.\AmmyCustomizer.h
# End Source File
# Begin Source File

SOURCE=.\AmmyCustomizer.rc
# End Source File
# Begin Source File

SOURCE=.\DlgCustomizer.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgCustomizer.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\AmmyCustomizer.ico
# End Source File
# Begin Source File

SOURCE=.\res\AmmyCustomizer.rc2
# End Source File
# Begin Source File

SOURCE=.\res\AmmyyCustomizer.ico
# End Source File
# Begin Source File

SOURCE=.\res\AmmyyCustomizer2.ico
# End Source File
# Begin Source File

SOURCE=.\res\database_gear.ico
# End Source File
# Begin Source File

SOURCE=.\res\settings1_16x16.ico
# End Source File
# End Group
# Begin Group "RL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\RL\RLEncryptor01.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLEncryptor01.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLEncryptor02.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLEncryptor02.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLException.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLException.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLFile.cpp
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

SOURCE=..\RL\RLResource.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLResource.h
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

SOURCE=..\RL\RLWnd.h
# End Source File
# Begin Source File

SOURCE=..\RL\StringA.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\StringA.h
# End Source File
# Begin Source File

SOURCE=..\RL\StringW.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\StringW.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\Common2.cpp
# End Source File
# Begin Source File

SOURCE=..\main\DlgOperatorPermissions.cpp
# End Source File
# Begin Source File

SOURCE=..\main\DlgOperatorPermissions.h
# End Source File
# Begin Source File

SOURCE=..\main\DlgPermissionList.cpp
# End Source File
# Begin Source File

SOURCE=..\main\DlgPermissionList.h
# End Source File
# Begin Source File

SOURCE=..\common\MD5.cpp
# End Source File
# Begin Source File

SOURCE=..\main\RLLanguages.cpp
# End Source File
# Begin Source File

SOURCE=..\main\RLLanguages.h
# End Source File
# Begin Source File

SOURCE=..\main\RLLanguages_data.h
# End Source File
# End Group
# End Target
# End Project
