# Microsoft Developer Studio Project File - Name="metamod" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=metamod - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "metamod.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "metamod.mak" CFG="metamod - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "metamod - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "metamod - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "metamod - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "release"
# PROP Intermediate_Dir "release\obj"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "METAMOD_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".." /I "..\..\..\hlsdk\common" /I "..\..\..\hlsdk\engine" /I "..\..\..\hlsdk\dlls" /I "..\..\..\hlsdk\pm_shared" /I "..\..\..\hlsdk\public" /D "METAMOD_CORE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "METAMOD_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"release/inf/metamod.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:none /machine:I386 /def:".\metamod.def"
# SUBTRACT LINK32 /verbose

!ELSEIF  "$(CFG)" == "metamod - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "debug"
# PROP Intermediate_Dir "debug\obj"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "METAMOD_EXPORTS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I ".." /I "..\..\..\hlsdk\common" /I "..\..\..\hlsdk\engine" /I "..\..\..\hlsdk\dlls" /I "..\..\..\hlsdk\pm_shared" /I "..\..\..\hlsdk\public" /D "METAMOD_CORE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "METAMOD_EXPORTS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"debug/inf/metamod.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"debug/inf/metamod.pdb" /debug /machine:I386 /def:".\metamod.def" /pdbtype:sept
# SUBTRACT LINK32 /map

!ENDIF 

# Begin Target

# Name "metamod - Win32 Release"
# Name "metamod - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\api_info.cpp
# End Source File
# Begin Source File

SOURCE=..\commands_meta.cpp
# End Source File
# Begin Source File

SOURCE=..\conf_meta.cpp
# End Source File
# Begin Source File

SOURCE=..\dllapi.cpp
# End Source File
# Begin Source File

SOURCE=..\engine_api.cpp
# End Source File
# Begin Source File

SOURCE=..\engineinfo.cpp
# End Source File
# Begin Source File

SOURCE=..\game_support.cpp
# End Source File
# Begin Source File

SOURCE=..\h_export.cpp
# End Source File
# Begin Source File

SOURCE=..\linkent.cpp
# End Source File
# Begin Source File

SOURCE=..\linkgame.cpp
# End Source File
# Begin Source File

SOURCE=..\linkplug.cpp
# End Source File
# Begin Source File

SOURCE=..\log_meta.cpp
# End Source File
# Begin Source File

SOURCE=..\meta_eiface.cpp
# End Source File
# Begin Source File

SOURCE=..\metamod.cpp
# End Source File
# Begin Source File

SOURCE=..\mhook.cpp
# End Source File
# Begin Source File

SOURCE=..\mlist.cpp
# End Source File
# Begin Source File

SOURCE=..\mplayer.cpp
# End Source File
# Begin Source File

SOURCE=..\mplugin.cpp
# End Source File
# Begin Source File

SOURCE=..\mqueue.cpp
# End Source File
# Begin Source File

SOURCE=..\mreg.cpp
# End Source File
# Begin Source File

SOURCE=..\mutil.cpp
# End Source File
# Begin Source File

SOURCE=..\osdep.cpp
# End Source File
# Begin Source File

SOURCE=..\reg_support.cpp
# End Source File
# Begin Source File

SOURCE=..\sdk_util.cpp
# End Source File
# Begin Source File

SOURCE=..\studioapi.cpp
# End Source File
# Begin Source File

SOURCE=..\support_meta.cpp
# End Source File
# Begin Source File

SOURCE=..\thread_logparse.cpp
# End Source File
# Begin Source File

SOURCE=..\vdate.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\api_info.h
# End Source File
# Begin Source File

SOURCE=..\commands_meta.h
# End Source File
# Begin Source File

SOURCE=..\conf_meta.h
# End Source File
# Begin Source File

SOURCE=..\dllapi.h
# End Source File
# Begin Source File

SOURCE=..\engine_api.h
# End Source File
# Begin Source File

SOURCE=..\engine_t.h
# End Source File
# Begin Source File

SOURCE=..\enginecallbacks.h
# End Source File
# Begin Source File

SOURCE=..\engineinfo.h
# End Source File
# Begin Source File

SOURCE=..\game_support.h
# End Source File
# Begin Source File

SOURCE=..\games.h
# End Source File
# Begin Source File

SOURCE=..\h_export.h
# End Source File
# Begin Source File

SOURCE=..\info_name.h
# End Source File
# Begin Source File

SOURCE=..\linkent.h
# End Source File
# Begin Source File

SOURCE=..\log_meta.h
# End Source File
# Begin Source File

SOURCE=..\meta_api.h
# End Source File
# Begin Source File

SOURCE=..\meta_eiface.h
# End Source File
# Begin Source File

SOURCE=..\metamod.h
# End Source File
# Begin Source File

SOURCE=..\mhook.h
# End Source File
# Begin Source File

SOURCE=..\mlist.h
# End Source File
# Begin Source File

SOURCE=..\mplayer.h
# End Source File
# Begin Source File

SOURCE=..\mplugin.h
# End Source File
# Begin Source File

SOURCE=..\mqueue.h
# End Source File
# Begin Source File

SOURCE=..\mreg.h
# End Source File
# Begin Source File

SOURCE=..\mutil.h
# End Source File
# Begin Source File

SOURCE=..\osdep.h
# End Source File
# Begin Source File

SOURCE=..\plinfo.h
# End Source File
# Begin Source File

SOURCE=..\reg_support.h
# End Source File
# Begin Source File

SOURCE=..\sdk_util.h
# End Source File
# Begin Source File

SOURCE=..\studioapi.h
# End Source File
# Begin Source File

SOURCE=..\support_meta.h
# End Source File
# Begin Source File

SOURCE=..\thread_logparse.h
# End Source File
# Begin Source File

SOURCE=..\tqueue.h
# End Source File
# Begin Source File

SOURCE=..\types_meta.h
# End Source File
# Begin Source File

SOURCE=..\vdate.h
# End Source File
# Begin Source File

SOURCE=..\vers_meta.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\res_meta.rc
# End Source File
# End Group
# End Target
# End Project
