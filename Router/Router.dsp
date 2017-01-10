# Microsoft Developer Studio Project File - Name="Router" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Router - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Router.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Router.mak" CFG="Router - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Router - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Router - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Router - Win32 Release"

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
# ADD LINK32 Ws2_32.lib User32.lib Gdi32.lib /nologo /subsystem:windows /map /machine:I386 /out:"../../_bin/AMMYY_Router.exe"

!ELSEIF  "$(CFG)" == "Router - Win32 Debug"

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
# ADD LINK32 Ws2_32.lib Iphlpapi.lib User32.lib Gdi32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../../_bin/AMMYY_Router.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Router - Win32 Release"
# Name "Router - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;h"
# Begin Source File

SOURCE=..\main\aaProtocol.h
# End Source File
# Begin Source File

SOURCE=..\main\CmdBase.cpp
# End Source File
# Begin Source File

SOURCE=..\main\CmdBase.h
# End Source File
# Begin Source File

SOURCE=..\main\CmdMsgBox.h
# End Source File
# Begin Source File

SOURCE=.\CmdRouterInit.h
# End Source File
# Begin Source File

SOURCE=..\main\CmdSessionEnded.cpp
# End Source File
# Begin Source File

SOURCE=..\main\CmdSessionEnded.h
# End Source File
# Begin Source File

SOURCE=.\CmdWaitingIDs.h
# End Source File
# Begin Source File

SOURCE=..\main\Common.cpp
# End Source File
# Begin Source File

SOURCE=..\main\Common.h
# End Source File
# Begin Source File

SOURCE=..\common\Common2.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Common2.h
# End Source File
# Begin Source File

SOURCE=.\CRouter.cpp
# End Source File
# Begin Source File

SOURCE=.\CRouter.h
# End Source File
# Begin Source File

SOURCE=..\main\DlgAddComputer.h
# End Source File
# Begin Source File

SOURCE=.\DlgMain.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgMain.h
# End Source File
# Begin Source File

SOURCE=.\FastQueue.cpp
# End Source File
# Begin Source File

SOURCE=.\FastQueue.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLEvent.h
# End Source File
# Begin Source File

SOURCE=.\RLHttp2.cpp
# End Source File
# Begin Source File

SOURCE=.\RLHttp2.h
# End Source File
# Begin Source File

SOURCE=.\Router.rc
# End Source File
# Begin Source File

SOURCE=.\RouterApp.cpp
# End Source File
# Begin Source File

SOURCE=.\RouterApp.h
# End Source File
# Begin Source File

SOURCE=.\Service.cpp
# End Source File
# Begin Source File

SOURCE=.\Service.h
# End Source File
# Begin Source File

SOURCE=..\main\ServiceManager.cpp
# End Source File
# Begin Source File

SOURCE=..\main\ServiceManager.h
# End Source File
# Begin Source File

SOURCE=.\Sockets.cpp
# End Source File
# Begin Source File

SOURCE=.\Sockets.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StreamWithTime.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamWithTime.h
# End Source File
# Begin Source File

SOURCE=.\TaskSessionEnded.cpp
# End Source File
# Begin Source File

SOURCE=.\TaskSessionEnded.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\binary1.bin
# End Source File
# Begin Source File

SOURCE=.\res\login.htm
# End Source File
# Begin Source File

SOURCE=.\res\main.html
# End Source File
# Begin Source File

SOURCE=.\res\TCPTunnel.ico
# End Source File
# Begin Source File

SOURCE=.\res\TCPTunnel.rc2
# End Source File
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

SOURCE=..\RL\RLEH.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLEH.h
# End Source File
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

SOURCE=..\RL\RLFile.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLHardwareMac.h
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

SOURCE=..\RL\RLRegistry.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLRegistry.h
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
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\md5.cpp
# End Source File
# Begin Source File

SOURCE=..\common\md5.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\login.html
# End Source File
# End Target
# End Project
