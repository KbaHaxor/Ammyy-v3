# Microsoft Developer Studio Project File - Name="Main" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Main - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "main.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "main.mak" CFG="Main - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Main - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Main - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Main - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../_bin/main/Release"
# PROP Intermediate_Dir "../../_bin/main/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W2 /GX /O1 /I "../vtc_common/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fr /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /i "../viewer/res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 Version.lib Ws2_32.lib Gdi32.lib User32.lib shell32.lib libjpeg.lib zlib.lib libspeex.lib /nologo /subsystem:windows /map /machine:I386 /out:"../../_bin/AMMYY_Admin.exe" /libpath:"../common/_libs/Release"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "Main - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../_bin/main/Debug"
# PROP Intermediate_Dir "../../_bin/main/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "../vtc_common/" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /i "../viewer/res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Iphlpapi.lib Version.lib Ws2_32.lib Gdi32.lib User32.lib shell32.lib libjpeg.lib zlib.lib libspeex.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../../_bin/AMMYY_Admin.exe" /pdbtype:sept /libpath:"../common/_libs/Debug/"

!ENDIF 

# Begin Target

# Name "Main - Win32 Release"
# Name "Main - Win32 Debug"
# Begin Group "main"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;h"
# Begin Group "proxy"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\proxy\HttpBasicAuthObject.cpp
# End Source File
# Begin Source File

SOURCE=.\proxy\HttpBasicAuthObject.h
# End Source File
# Begin Source File

SOURCE=.\proxy\HttpClient.cpp
# End Source File
# Begin Source File

SOURCE=.\proxy\HttpClient.h
# End Source File
# Begin Source File

SOURCE=.\proxy\HttpNtlmAuthObject.cpp
# End Source File
# Begin Source File

SOURCE=.\proxy\HttpNtlmAuthObject.h
# End Source File
# End Group
# Begin Group "rsa"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\rsa\bignum.cpp
# End Source File
# Begin Source File

SOURCE=.\rsa\bignum.h
# End Source File
# Begin Source File

SOURCE=.\rsa\bn_mul.h
# End Source File
# Begin Source File

SOURCE=.\rsa\config.h
# End Source File
# Begin Source File

SOURCE=.\rsa\rsa.cpp
# End Source File
# Begin Source File

SOURCE=.\rsa\rsa.h
# End Source File
# End Group
# Begin Group "res"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\images\resource_images.cpp
# End Source File
# Begin Source File

SOURCE=.\images\resource_images.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\AmmyyApp.cpp
# End Source File
# Begin Source File

SOURCE=.\AmmyyApp.h
# End Source File
# Begin Source File

SOURCE=.\CmdBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CmdBase.h
# End Source File
# Begin Source File

SOURCE=.\CmdGetRouterForID.cpp
# End Source File
# Begin Source File

SOURCE=.\CmdGetRouterForID.h
# End Source File
# Begin Source File

SOURCE=.\CmdInit.cpp
# End Source File
# Begin Source File

SOURCE=.\CmdInit.h
# End Source File
# Begin Source File

SOURCE=.\CmdMsgBox.cpp
# End Source File
# Begin Source File

SOURCE=.\CmdMsgBox.h
# End Source File
# Begin Source File

SOURCE=.\CmdPortTest.cpp
# End Source File
# Begin Source File

SOURCE=.\CmdPortTest.h
# End Source File
# Begin Source File

SOURCE=.\CmdSessionEnded.cpp
# End Source File
# Begin Source File

SOURCE=.\CmdSessionEnded.h
# End Source File
# Begin Source File

SOURCE=.\Common.cpp
# End Source File
# Begin Source File

SOURCE=.\Common.h
# End Source File
# Begin Source File

SOURCE=.\Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\Debug.h
# End Source File
# Begin Source File

SOURCE=.\DlgAbout.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgAbout.h
# End Source File
# Begin Source File

SOURCE=.\DlgAddComputer.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgAddComputer.h
# End Source File
# Begin Source File

SOURCE=.\DlgContactBook.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgContactBook.h
# End Source File
# Begin Source File

SOURCE=.\DlgEncoder.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgEncoder.h
# End Source File
# Begin Source File

SOURCE=.\DlgEncoderList.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgEncoderList.h
# End Source File
# Begin Source File

SOURCE=.\DlgExternalPorts.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgExternalPorts.h
# End Source File
# Begin Source File

SOURCE=.\DlgHttpProxy.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgHttpProxy.h
# End Source File
# Begin Source File

SOURCE=.\DlgMain.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgMain.h
# End Source File
# Begin Source File

SOURCE=.\DlgOperatorPermissions.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgOperatorPermissions.h
# End Source File
# Begin Source File

SOURCE=.\DlgPermissionList.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgPermissionList.h
# End Source File
# Begin Source File

SOURCE=.\DlgRDPSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgRDPSettings.h
# End Source File
# Begin Source File

SOURCE=.\DlgSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgSettings.h
# End Source File
# Begin Source File

SOURCE=.\Downloader.cpp
# End Source File
# Begin Source File

SOURCE=.\Downloader.h
# End Source File
# Begin Source File

SOURCE=.\DynamicFn.cpp
# End Source File
# Begin Source File

SOURCE=.\DynamicFn.h
# End Source File
# Begin Source File

SOURCE=.\FastQueue.cpp
# End Source File
# Begin Source File

SOURCE=.\FastQueue.h
# End Source File
# Begin Source File

SOURCE=.\Image.h
# End Source File
# Begin Source File

SOURCE=.\ImpersonateWrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\ImpersonateWrapper.h
# End Source File
# Begin Source File

SOURCE=.\Installer.rc
# End Source File
# Begin Source File

SOURCE=.\InteropCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\InteropCommon.h
# End Source File
# Begin Source File

SOURCE=.\InteropTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\InteropTarget.h
# End Source File
# Begin Source File

SOURCE=.\InteropViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\InteropViewer.h
# End Source File
# Begin Source File

SOURCE=.\Light.cpp
# End Source File
# Begin Source File

SOURCE=.\Light.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ReTranslator.cpp
# End Source File
# Begin Source File

SOURCE=.\ReTranslator.h
# End Source File
# Begin Source File

SOURCE=.\RLLanguages.cpp
# End Source File
# Begin Source File

SOURCE=.\RLLanguages.h
# End Source File
# Begin Source File

SOURCE=.\RLLanguages_data.h
# End Source File
# Begin Source File

SOURCE=.\ServerInteract.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerInteract.h
# End Source File
# Begin Source File

SOURCE=.\Service.cpp
# End Source File
# Begin Source File

SOURCE=.\Service.h
# End Source File
# Begin Source File

SOURCE=.\ServiceManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ServiceManager.h
# End Source File
# Begin Source File

SOURCE=.\Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\Settings.h
# End Source File
# Begin Source File

SOURCE=.\StaticLink.cpp
# End Source File
# Begin Source File

SOURCE=.\StaticLink.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TCP.cpp
# End Source File
# Begin Source File

SOURCE=.\TCP.h
# End Source File
# Begin Source File

SOURCE=.\Transport.cpp
# End Source File
# Begin Source File

SOURCE=.\Transport.h
# End Source File
# Begin Source File

SOURCE=.\TransportT.cpp
# End Source File
# Begin Source File

SOURCE=.\TransportT.h
# End Source File
# Begin Source File

SOURCE=.\TSSessions.cpp
# End Source File
# Begin Source File

SOURCE=.\TSSessions.h
# End Source File
# Begin Source File

SOURCE=.\Vista.cpp
# End Source File
# Begin Source File

SOURCE=.\Vista.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\Ammyy_logo.ico
# End Source File
# Begin Source File

SOURCE=..\binary1.bin
# End Source File
# Begin Source File

SOURCE=.\res\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn3.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_border.bmp
# End Source File
# Begin Source File

SOURCE=.\res\contactbook_0.bmp
# End Source File
# Begin Source File

SOURCE=.\res\contactbook_1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\contactbook_2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\contactbook_3.bmp
# End Source File
# Begin Source File

SOURCE=.\res\export.bmp
# End Source File
# Begin Source File

SOURCE=.\res\import.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Installer.ico
# End Source File
# Begin Source File

SOURCE=.\res\Installer.rc2
# End Source File
# Begin Source File

SOURCE=.\manifest.bin
# End Source File
# Begin Source File

SOURCE=..\rl_manif.bin
# End Source File
# Begin Source File

SOURCE=.\res\test.bmp
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

SOURCE=..\RL\RLEvent.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLEvent.h
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

SOURCE=..\RL\RLHardwareHDD.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLHardwareHDD_ddk.h
# End Source File
# Begin Source File

SOURCE=..\RL\RLHardwareMac.h
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

SOURCE=..\RL\RLSheet.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\RLSheet.h
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

SOURCE=..\RL\other\RLToolTip.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\other\RLToolTip.h
# End Source File
# Begin Source File

SOURCE=..\RL\other\RLToolTipButton.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\other\RLToolTipButton.h
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
# Begin Source File

SOURCE=..\RL\other\TextProgressCtrl.cpp
# End Source File
# Begin Source File

SOURCE=..\RL\other\TextProgressCtrl.h
# End Source File
# End Group
# Begin Group "viewer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\viewer\vrClient.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\vrClient.h
# End Source File
# Begin Source File

SOURCE=..\viewer\vrClientClipboard.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\vrClientCopyRect.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\vrClientCursor.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\vrClientFullScreen.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\VrDlgPasswordInput.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\VrDlgPasswordInput.h
# End Source File
# Begin Source File

SOURCE=..\viewer\VrDlgSpeedTest.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\VrDlgSpeedTest.h
# End Source File
# Begin Source File

SOURCE=..\viewer\VrEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\VrEncoder.h
# End Source File
# Begin Source File

SOURCE=..\viewer\VrEncoderAAC.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\VrEncoderHextile.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\VrEncoderJpeg.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\VrFm1.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\VrFm1.h
# End Source File
# Begin Source File

SOURCE=..\viewer\VrFmBlockR.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\VrFmBlockR.h
# End Source File
# Begin Source File

SOURCE=..\viewer\vrHelp.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\vrHelp.h
# End Source File
# Begin Source File

SOURCE=..\viewer\vrHotKeys.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\vrHotKeys.h
# End Source File
# Begin Source File

SOURCE=..\viewer\vrKeyMap.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\vrKeyMap.h
# End Source File
# Begin Source File

SOURCE=..\viewer\vrMain.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\vrMain.h
# End Source File
# Begin Source File

SOURCE=..\viewer\vrOptions.cpp
# End Source File
# Begin Source File

SOURCE=..\viewer\vrOptions.h
# End Source File
# End Group
# Begin Group "target"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\target\TrClient.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrClient.h
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktop.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktop.h
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktopCapture.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktopCapture.h
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktopComparator.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktopComparator.h
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktopCopyRect.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktopCopyRect.h
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktopUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrDesktopUtils.h
# End Source File
# Begin Source File

SOURCE=..\target\TrDlgAccept.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrDlgAccept.h
# End Source File
# Begin Source File

SOURCE=..\target\TrEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrEncoder.h
# End Source File
# Begin Source File

SOURCE=..\target\TrEncoderAAC.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrEncoderAAC.h
# End Source File
# Begin Source File

SOURCE=..\target\TrEncoderAACpalette.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrEncoderAACpalette.h
# End Source File
# Begin Source File

SOURCE=..\target\TrEncoderHexT.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrEncoderHexT.h
# End Source File
# Begin Source File

SOURCE=..\target\TrFm.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrFm.h
# End Source File
# Begin Source File

SOURCE=..\target\TrFmFileSys.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrFmFileSys.h
# End Source File
# Begin Source File

SOURCE=..\target\TrJpegCompressor.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrJpegCompressor.h
# End Source File
# Begin Source File

SOURCE=..\target\TrKeyDef.h
# End Source File
# Begin Source File

SOURCE=..\target\TrKeymap.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrKeymap.h
# End Source File
# Begin Source File

SOURCE=..\target\TrListener.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrListener.h
# End Source File
# Begin Source File

SOURCE=..\target\TrMain.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrMain.h
# End Source File
# Begin Source File

SOURCE=..\target\TrRegion.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrRegion.h
# End Source File
# Begin Source File

SOURCE=..\target\TrService.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrService.h
# End Source File
# Begin Source File

SOURCE=..\target\TrTranslate.cpp
# End Source File
# Begin Source File

SOURCE=..\target\TrTranslate.h
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\aaDesktop.h
# End Source File
# Begin Source File

SOURCE=.\aaProtocol.h
# End Source File
# Begin Source File

SOURCE=..\common\Common2.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Common2.h
# End Source File
# Begin Source File

SOURCE=..\common\Debug2.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Debug2.h
# End Source File
# Begin Source File

SOURCE=..\common\unzip\LiteUnzip.cpp
# End Source File
# Begin Source File

SOURCE=..\common\unzip\LiteUnzip.h
# End Source File
# Begin Source File

SOURCE=..\common\md5.cpp
# End Source File
# Begin Source File

SOURCE=..\common\md5.h
# End Source File
# Begin Source File

SOURCE=..\common\omnithread\nt.cpp
# End Source File
# Begin Source File

SOURCE=..\common\omnithread\nt.h
# End Source File
# Begin Source File

SOURCE=..\common\omnithread\omnithread.h
# End Source File
# Begin Source File

SOURCE=..\common\unzip\Unzip.cpp
# End Source File
# Begin Source File

SOURCE=..\common\unzip\Unzip.h
# End Source File
# Begin Source File

SOURCE=..\common\vtcLog.cpp
# End Source File
# Begin Source File

SOURCE=..\common\vtcLog.h
# End Source File
# Begin Source File

SOURCE=..\common\XLOCK.CPP
# End Source File
# Begin Source File

SOURCE=..\common\YVALS_my.H
# End Source File
# End Group
# Begin Group "sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sound\AudioIn.cpp
# End Source File
# Begin Source File

SOURCE=.\sound\AudioIn.h
# End Source File
# Begin Source File

SOURCE=.\sound\AudioOut.cpp
# End Source File
# Begin Source File

SOURCE=.\sound\AudioOut.h
# End Source File
# End Group
# Begin Group "udt"

# PROP Default_Filter ""
# End Group
# Begin Source File

SOURCE=..\..\Ammyy.txt
# End Source File
# Begin Source File

SOURCE=..\ream_me.txt
# End Source File
# End Target
# End Project
