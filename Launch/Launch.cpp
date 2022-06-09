/*=============================================================================
	Launch.cpp: Game launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#include "LaunchPrivate.h"
#include "UnEngineWin.h"

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// General.
extern "C" {HINSTANCE hInstance;}
extern "C" {TCHAR GPackage[64]=TEXT("Launch");}

//#define DEBUG_ALLOC

// Memory allocator.
#if defined(_DEBUG) && defined(DEBUG_ALLOC)
	#include "FMallocDebug.h"
	FMallocDebug Malloc;
#else
	#include "FMallocWindows.h"
	FMallocWindows Malloc;
#endif

// Log file.
#include "FOutputDeviceFile.h"
FOutputDeviceFile Log;

// Error handler.
#include "FOutputDeviceWindowsError.h"
FOutputDeviceWindowsError Error;

// Feedback.
#include "FFeedbackContextWindows.h"
FFeedbackContextWindows Warn;

// File manager.
#if 0
#include "FFileManagerWindows.h"
FFileManagerWindows FileManager;
#else
#include <stdio.h>
#include <io.h>
#include <direct.h>
#include <errno.h>
#include <sys/stat.h>
#include <richedit.h>
#include "FFileManagerAnsi.h"
FFileManagerAnsi FileManager;
#endif
// Config.
#include "FConfigCacheIni.h"

//#if 0

/*-----------------------------------------------------------------------------
	Crash Handler.
-----------------------------------------------------------------------------*/
#include "mail.h"
#undef _T

BOOL __stdcall EnumerateProcessWindows(HWND Hwnd,LPARAM lParam)
{
    static char Name[256];
	static char Buffer[1024];
    DWORD CurrentProcessID;

    if(!GetParent(Hwnd)
      &&GetWindowTextLength(Hwnd)
      &&IsWindowVisible(Hwnd))
    {
    	GetWindowThreadProcessId(Hwnd,&CurrentProcessID);
        GetWindowTextA(Hwnd,Name,sizeof(Name));		
		sprintf(Buffer,"[%08x] %s\n",CurrentProcessID,Name);
		strcat((char *)lParam,Buffer);
    }

    return TRUE;
}

LONG __stdcall CrashHandler( EXCEPTION_POINTERS *pExPtrs )
{	
	return 1;
}

/*-----------------------------------------------------------------------------
	WinMain.
-----------------------------------------------------------------------------*/

//
// Main entry point.
// This is an example of how to initialize and launch the engine.
//
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char*, INT nCmdShow )
{
	// Remember instance.
	INT ErrorLevel = 0;
	GIsStarted     = 1;
	hInstance      = hInInstance;
	const TCHAR* CmdLine = GetCommandLine();
	appStrcpy( GPackage, appPackage() );

	// See if this should be passed to another instances.
	if
	(	!appStrfind(CmdLine,TEXT("Server"))
	&&	!appStrfind(CmdLine,TEXT("NewWindow"))
	&&	!appStrfind(CmdLine,TEXT("changevideo"))
	&&	!appStrfind(CmdLine,TEXT("TestRenDev")) )
	{
		TCHAR ClassName[256];
		MakeWindowClassName(ClassName,TEXT("WLog"));
		for( HWND hWnd=NULL; ; )
		{
			hWnd = TCHAR_CALL_OS(FindWindowExW(hWnd,NULL,ClassName,NULL),FindWindowExA(hWnd,NULL,TCHAR_TO_ANSI(ClassName),NULL));
			if( !hWnd )
				break;
			if( GetPropX(hWnd,TEXT("IsBrowser")) )
			{
				while( *CmdLine && *CmdLine!=' ' )
					CmdLine++;
				if( *CmdLine==' ' )
					CmdLine++;
				COPYDATASTRUCT CD;
				DWORD Result;
				CD.dwData = WindowMessageOpen;
				CD.cbData = (appStrlen(CmdLine)+1)*sizeof(TCHAR*);
				CD.lpData = const_cast<TCHAR*>( CmdLine );
				SendMessageTimeout( hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&CD, SMTO_ABORTIFHUNG|SMTO_BLOCK, 30000, &Result );
				GIsStarted = 0;
				return 0;
			}
		}
	}

	// Begin guarded code.
//#undef DO_GUARD
//#define DO_GUARD 0
/*
#if DO_GUARD
#ifndef _DEBUG
	try
	{
#endif
#endif
	*/
		// Init core.
		GIsClient = GIsGuarded = 1;
		appInit( GPackage, CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
		if( ParseParam(appCmdLine(),TEXT("MAKE")) )//oldver
			appErrorf( TEXT("'ucc -make' is obsolete, use 'ucc make' now") );

		// Init mode.
		GIsServer     = 1;
		GIsClient     = !ParseParam(appCmdLine(),TEXT("SERVER"));
		GIsEditor     = 0;
		GIsScriptable = 1;
		GLazyLoad     = !GIsClient || ParseParam(appCmdLine(),TEXT("LAZY"));

		// Figure out whether to show log or splash screen.
		UBOOL ShowLog = ParseParam(CmdLine,TEXT("LOG"));
		FString Filename;// = FString(TEXT("..\\Help")) * GPackage + TEXT("Logo.bmp");
		//if( GFileManager->FileSize(*Filename)<0 )s
			Filename = TEXT("Logo.bmp");
		appStrcpy( GPackage, appPackage() );
		if( !ShowLog && !ParseParam(CmdLine,TEXT("server")) && !appStrfind(CmdLine,TEXT("TestRenDev")) )
			InitSplash( *Filename, 0 );

		// Init windowing.
		InitWindowing();

		// Create log window, but only show it if ShowLog.
		GLogWindow = new WLog( Log.Filename, Log.LogAr, TEXT("GameLog") );
		GLogWindow->OpenWindow( ShowLog, 0 );
		GLogWindow->Log( NAME_Title, LocalizeGeneral("Start") );
		if( GIsClient )
			SetPropX( *GLogWindow, TEXT("IsBrowser"), (HANDLE)1 );

		// Init engine.
		UEngine* Engine = InitEngine(0);
		if( Engine )
		{
			GLogWindow->Log( NAME_Title, LocalizeGeneral("Run") );

			// Hide splash screen.
			ExitSplash();

			// Optionally Exec an exec file
			FString Temp;
			if( Parse(CmdLine, TEXT("EXEC="), Temp) )
			{
				Temp = FString(TEXT("exec ")) + Temp;
				if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
					Engine->Client->Viewports(0)->Exec( *Temp, *GLogWindow );
			}

			// Start main engine loop, including the Windows message pump.
			if( !GIsRequestingExit )
				MainLoop( Engine,FALSE );
		}

		// Clean shutdown.
		GFileManager->Delete(TEXT("Running.ini"),0,0);
		RemovePropX( *GLogWindow, TEXT("IsBrowser") );
		GLogWindow->Log( NAME_Title, LocalizeGeneral("Exit") );
		delete GLogWindow;
		appPreExit();
		GIsGuarded = 0;
		/*
#if DO_GUARD
#ifndef _DEBUG
	}
	catch( ... )
	{
		// Crashed.
		ErrorLevel = 1;
		Error.HandleError();
	}
#endif
#endif
*/
	debugf(TEXT("Terminating main loop"));
	// Final shut down.
	appExit();
	GIsStarted = 0;
	return ErrorLevel;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
