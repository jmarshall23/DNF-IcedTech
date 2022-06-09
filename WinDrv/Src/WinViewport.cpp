/*=============================================================================
	UnWnCam.cpp: UWindowsViewport code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "..\..\Engine\Inc\EnginePrivate.h"

#define DD_OTHERLOCKFLAGS 0 /*DDLOCK_NOSYSLOCK*/ /*0x00000800L*/
#define WM_MOUSEWHEEL 0x020A

/*-----------------------------------------------------------------------------
	Class implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UWindowsViewport);

/*-----------------------------------------------------------------------------
	UWindowsViewport Init/Exit.
-----------------------------------------------------------------------------*/

// Constructor.
UWindowsViewport::UWindowsViewport()
:	UViewport()
,	Status( WIN_ViewportOpening )
{
	Window = new WWindowsViewportWindow( this );

	// Set color bytes based on screen resolution.
	HWND hwndDesktop = GetDesktopWindow();
	HDC  hdcDesktop  = GetDC(hwndDesktop);
	switch( GetDeviceCaps( hdcDesktop, BITSPIXEL ) )
	{
		case 8:
			ColorBytes  = 2;
			break;
		case 16:
			ColorBytes  = 2;
			Caps       |= CC_RGB565;
			break;
		case 24:
			ColorBytes  = 4;
			break;
		case 32: 
			ColorBytes  = 4;
			break;
		default: 
			ColorBytes  = 2; 
			Caps       |= CC_RGB565;
			break;
	}
	DesktopColorBytes = ColorBytes;

	// Init other stuff.
	ReleaseDC( hwndDesktop, hdcDesktop );
	SavedCursor.x = -1;

	StandardCursors[0] = LoadCursorIdX(NULL, IDC_ARROW);
	StandardCursors[1] = LoadCursorIdX(NULL, IDC_SIZEALL);
	StandardCursors[2] = LoadCursorIdX(NULL, IDC_SIZENESW);
	StandardCursors[3] = LoadCursorIdX(NULL, IDC_SIZENS);
	StandardCursors[4] = LoadCursorIdX(NULL, IDC_SIZENWSE);
	StandardCursors[5] = LoadCursorIdX(NULL, IDC_SIZEWE);
	StandardCursors[6] = LoadCursorIdX(NULL, IDC_WAIT);
}

// Destroy:
void UWindowsViewport::Destroy()
{
	Super::Destroy();

	diShutdownKeyboardMouse();
	if( BlitFlags & BLIT_Temporary ) { appFree( ScreenPointer ); ScreenPointer=NULL; }
	
	check(Window);
	Window->MaybeDestroy();
	delete Window; Window = NULL;
}

// Error shutdown:
void UWindowsViewport::ShutdownAfterError()
{
	if( ddBackBuffer )	{ ddBackBuffer->Release(); ddBackBuffer=NULL; }
	if( ddFrontBuffer ) { ddFrontBuffer->Release(); ddFrontBuffer=NULL; }
	if( Window->hWnd )	{ DestroyWindow( Window->hWnd ); Window->hWnd=NULL; }

	Super::ShutdownAfterError();
}

/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

// Command line.
UBOOL UWindowsViewport::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	if( UViewport::Exec( Cmd, Ar ) )
	{
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("EndFullscreen")) )
	{
		EndFullscreen();
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("ToggleFullscreen")) )
	{
		ToggleFullscreen();
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetCurrentRes")) )
	{
		Ar.Logf( TEXT("%ix%i"), SizeX, SizeY, (ColorBytes?ColorBytes:2)*8 );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetCurrentColorDepth")) )
	{
		Ar.Logf( TEXT("%i"), (ColorBytes?ColorBytes:2)*8 );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetColorDepths")) )
	{
		Ar.Log( TEXT("16 32") );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetCurrentRenderDevice")) )
	{
		Ar.Log( RenDev->GetClass()->GetPathName() );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GetRes")) )
	{
		if( BlitFlags & BLIT_DirectDraw )
		{
			// DirectDraw modes.
			FString Result;
			for( INT i=0; i<GetOuterUWindowsClient()->DirectDrawModes[ColorBytes].Num(); i++ )
				Result += FString::Printf( TEXT("%ix%i "), (INT)GetOuterUWindowsClient()->DirectDrawModes[ColorBytes](i).X, (INT)GetOuterUWindowsClient()->DirectDrawModes[ColorBytes](i).Y );
			if( Result.Right(1)==TEXT(" ") )
				Result = Result.LeftChop(1);
			Ar.Log( *Result );
		}
		else if( BlitFlags & BLIT_DibSection )
		{
			// Windowed mode.
			Ar.Log( TEXT("320x240 400x300 512x384 640x480 800x600") );
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("SetRes")) )
	{
		INT X=appAtoi(Cmd);
		TCHAR* CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
		INT Y=appAtoi(CmdTemp);
		Cmd = CmdTemp;
		CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
		INT C=appAtoi(CmdTemp);
		INT NewColorBytes = C ? C/8 : ColorBytes;
		if( X && Y )
		{
			HoldCount++;
			UBOOL Result = RenDev->SetRes( X, Y, NewColorBytes, IsFullscreen() );
			HoldCount--;
			if( !Result )
				EndFullscreen();
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("Preferences")) )
	{
		UWindowsClient* Client = GetOuterUWindowsClient();
		Client->ConfigReturnFullscreen = 0;
		if( BlitFlags & BLIT_Fullscreen )
		{
			EndFullscreen();
			Client->ConfigReturnFullscreen = 1;
		}
		if( !Client->ConfigProperties )
		{
			Client->ConfigProperties = new WConfigProperties( TEXT("Preferences"), LocalizeGeneral("AdvancedOptionsTitle",TEXT("Window")) );
			Client->ConfigProperties->SetNotifyHook( Client );
			Client->ConfigProperties->OpenWindow( Window->hWnd );
			Client->ConfigProperties->ForceRefresh();
		}
		GetOuterUWindowsClient()->ConfigProperties->Show(1);
		SetFocus( *GetOuterUWindowsClient()->ConfigProperties );
		return 1;
	}
	else return 0;
}

/*-----------------------------------------------------------------------------
	Window openining and closing.
-----------------------------------------------------------------------------*/

// Open this viewport's window.
void UWindowsViewport::OpenWindow( DWORD InParentWindow, UBOOL IsTemporary, INT NewX, INT NewY, INT OpenX, INT OpenY )
{
	check(Actor);
	check(!HoldCount);
	UBOOL DoRepaint=0, DoSetActive=0;
	UWindowsClient* C = GetOuterUWindowsClient();
	if( NewX!=INDEX_NONE )
		NewX = Align( NewX, 2 );

	// User window of launcher if no parent window was specified.
	if( !InParentWindow )
		Parse( appCmdLine(), TEXT("HWND="), InParentWindow );

	// Create frame buffer.
	if( 0 && IsTemporary )
	{
		// Create in-memory data.
		BlitFlags     = BLIT_Temporary;
		ColorBytes    = 2;
		SizeX         = NewX;
		SizeY         = NewY;
		ScreenPointer = (BYTE*)appMalloc( 2 * NewX * NewY, TEXT("TemporaryViewportData") );	
		Window->hWnd  = NULL;
		debugf( NAME_Log, TEXT("Opened temporary viewport") );
   	} else
	{
		// Figure out physical window size we must specify to get appropriate client area.
		FRect rTemp( 100, 100, (NewX!=INDEX_NONE?NewX:C->WindowedViewportX) + 100, (NewY!=INDEX_NONE?NewY:C->WindowedViewportY) + 100 );

		// Get style and proper rectangle.
		DWORD Style;
		if( InParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
		{
			Style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS;
   			AdjustWindowRect( rTemp, Style, 0 );
		}
		else
		{
			Style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
   			AdjustWindowRect( rTemp, Style, (Actor->ShowFlags & SHOW_Menu) ? TRUE : FALSE );
		}

		// Set position and size.
		if( OpenX==-1 )	OpenX = CW_USEDEFAULT;
		if( OpenY==-1 ) OpenY = CW_USEDEFAULT;
		INT OpenXL = rTemp.Width();
		INT OpenYL = rTemp.Height();

		// Create or update the window.
		if( !Window->hWnd )
		{
			// Creating new viewport.
			ParentWindow = (HWND)InParentWindow;

			// Open the physical window.
			Window->PerformCreateWindowEx
			(
				WS_EX_APPWINDOW,
				TEXT(""),
				Style,
				OpenX, OpenY,
				OpenXL, OpenYL,
				ParentWindow,
				NULL,
				hInstance
			);

			// Set parent window.
			if( ParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
			{
				// Force this to be a child.
				SetWindowLongX( Window->hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP );
			}
			DoSetActive = DoRepaint = 1;
		}
		else
		{
			// Resizing existing viewport.
			//!!only needed for old vb code
			SetWindowPos( Window->hWnd, NULL, OpenX, OpenY, OpenXL, OpenYL, SWP_NOACTIVATE );
		}
		ShowWindow( Window->hWnd, SW_SHOWNOACTIVATE );
		if( DoRepaint )
			UpdateWindow( Window->hWnd );
	}

// jmarshall - cleaned this up and forced windowed in debug so i can work.
	bool isFullscreen = !GIsEditor && GetOuterUWindowsClient()->StartupFullscreen;

#ifdef _DEBUG
	isFullscreen = false;
#endif

	if( !RenDev )
	{
		debugf( NAME_Log, TEXT("Trying D3DDrv.D3DRenderDevice") );
		TryRenderDevice( TEXT("D3DDrv.D3DRenderDevice"), NewX, NewY, IsTemporary?ColorBytes:INDEX_NONE, isFullscreen);
	}
	if(!RenDev)
	{
		appMsgf(TEXT("Could not set render device ... attempting to revert to software, please report this to Nick. (%ix%i), Fullscreen:%i, ColorBytes:%i"),NewX,NewY, isFullscreen,IsTemporary?ColorBytes:INDEX_NONE );
		debugf( NAME_Log, TEXT("Trying SoftDrv.SoftwareRenderDevice") );
		TryRenderDevice( TEXT("SoftDrv.SoftwareRenderDevice"), NewX, NewY, IsTemporary?ColorBytes:INDEX_NONE, isFullscreen);
	}
// jmarshall end
	check(RenDev);
	UpdateWindowFrame();
	if(DoRepaint)	Repaint( 1 );
	if (DoSetActive) {
		SetActiveWindow(Window->hWnd);
	}
}

// Close a viewport window.  Assumes that the viewport has been openened with
// OpenViewportWindow.  Does not affect the viewport's object, only the
// platform-specific information associated with it.
void UWindowsViewport::CloseWindow()
{
	if( Window->hWnd && Status==WIN_ViewportNormal )
	{
		Status = WIN_ViewportClosing;
		DestroyWindow( Window->hWnd );
	}
}

/*-----------------------------------------------------------------------------
	UWindowsViewport operations.
-----------------------------------------------------------------------------*/

// Set window position according to menu's on-top setting:
void UWindowsViewport::SetTopness()
{
	HWND Topness = HWND_NOTOPMOST;
	if( GetMenu(Window->hWnd) && GetMenuState(GetMenu(Window->hWnd),ID_ViewTop,MF_BYCOMMAND)&MF_CHECKED )
		Topness = HWND_TOPMOST;
	SetWindowPos( Window->hWnd, Topness, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE );
}

// Repaint the viewport.
void UWindowsViewport::Repaint( UBOOL Blit )
{
	GetOuterUWindowsClient()->Engine->Draw( this, Blit );
}

// Return whether fullscreen.
UBOOL UWindowsViewport::IsFullscreen()
{
	return (BlitFlags & BLIT_Fullscreen)!=0;
}

//
// Set the mouse cursor according to Unreal or UnrealEd's mode, or to
// an hourglass if a slow task is active.
//
void UWindowsViewport::SetModeCursor()
{
	enum EEditorMode
	{
		EM_None 			= 0,	// Gameplay, editor disabled.
		EM_ViewportMove		= 1,	// Move viewport normally.
		EM_ViewportZoom		= 2,	// Move viewport with acceleration.
		EM_BrushRotate		= 5,	// Rotate brush.
		EM_BrushScale		= 8,	// Stretch brush.
		EM_TexturePan		= 11,	// Pan textures.
		EM_TextureRotate	= 13,	// Rotate textures.
		EM_TextureScale		= 14,	// Scale textures.
		EM_BrushSnap		= 18,	// Brush snap-scale.
		EM_TexView			= 19,	// Viewing textures.
		EM_TexBrowser		= 20,	// Browsing textures.
		EM_MeshView			= 21,	// Viewing mesh.
		EM_MeshBrowser		= 22,	// Browsing mesh.
		EM_BrushClip		= 23,	// brush Clipping.
		EM_VertexEdit		= 24,	// Multiple Vertex Editing.
		EM_FaceDrag			= 25,	// Face Dragging.
		EM_Polygon			= 26,	// Free hand polygon drawing
		EM_TerrainEdit		= 27,	// Terrain editing.
	};
	if( GIsSlowTask )
	{
		SetCursor(LoadCursorIdX(NULL,IDC_WAIT));
		return;
	}
	HCURSOR hCursor;
	INT EM = GetOuterUWindowsClient()->Engine->edcamMode(this);
	switch( EM )
	{
		case EM_ViewportZoom:	hCursor = LoadCursorIdX(hInstance,IDCURSOR_CameraZoom); break;
		case EM_BrushRotate:	hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushRot); break;
		case EM_BrushScale:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushScale); break;
		case EM_BrushSnap:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushSnap); break;
		case EM_TexturePan:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexPan); break;
		case EM_TextureRotate:	hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexRot); break;
		case EM_TextureScale:	hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexScale); break;
		case EM_None: 			hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		case EM_ViewportMove: 	hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		case EM_TexView:		hCursor = LoadCursorIdX(NULL,IDC_ARROW); break;
		case EM_TexBrowser:		hCursor = LoadCursorIdX(NULL,IDC_ARROW); break;
		case EM_MeshView:		hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		case EM_VertexEdit: 	hCursor = LoadCursorIdX(hInstance,IDCURSOR_VertexEdit); break;
		case EM_BrushClip:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushClip); break;
		case EM_FaceDrag:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_FaceDrag); break;
		case EM_Polygon:	 	hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushWarp); break;
		case EM_TerrainEdit: 	hCursor = LoadCursorIdX(hInstance,IDCURSOR_TerrainEdit); break;
		default: 				hCursor = LoadCursorIdX(NULL,IDC_ARROW); break;
	}
	SetCursor( hCursor );
}

// Update user viewport interface.
void UWindowsViewport::UpdateWindowFrame()
{
	// If not a window, exit.
	if( HoldCount || !Window->hWnd || (BlitFlags&BLIT_Fullscreen) || (BlitFlags&BLIT_Temporary) )
		return;

	// Set viewport window's name to show resolution.
	TCHAR WindowName[80];
	if( !GIsEditor || (Actor->ShowFlags&SHOW_PlayerCtrl) )
	{
		appSprintf( WindowName, LocalizeGeneral("Product",appPackage()) );
	}
	else switch( Actor->RendMap )
	{
		case REN_Wire:		appStrcpy(WindowName,LocalizeGeneral("ViewPersp")); break;
		case REN_OrthXY:	appStrcpy(WindowName,LocalizeGeneral("ViewXY"   )); break;
		case REN_OrthXZ:	appStrcpy(WindowName,LocalizeGeneral("ViewXZ"   )); break;
		case REN_OrthYZ:	appStrcpy(WindowName,LocalizeGeneral("ViewYZ"   )); break;
		default:			appStrcpy(WindowName,LocalizeGeneral("ViewOther")); break;
	}
	Window->SetText( WindowName );

	// Update parent window.
	if( ParentWindow )
	{
		SendMessageX( ParentWindow, WM_CHAR, 0, 0 );
		PostMessageX( ParentWindow, WM_COMMAND, WM_VIEWPORT_UPDATEWINDOWFRAME, 0 );
	}
}

// Return the viewport's window.
void* UWindowsViewport::GetWindow()
{
	return Window->hWnd;
}

/*-----------------------------------------------------------------------------
	Input.
-----------------------------------------------------------------------------*/

// Input event router.
UBOOL UWindowsViewport::CauseInputEvent( INT iKey, EInputAction Action, FLOAT Delta )
{
	// Route to engine if a valid key; some keyboards produce key
	// codes that go beyond IK_MAX.
	if( iKey>=0 && iKey<IK_MAX )
		return GetOuterUWindowsClient()->Engine->InputEvent( this, (EInputKey)iKey, Action, Delta );
	else
		return 0;
}

//
// If the cursor is currently being captured, stop capturing, clipping, and 
// hiding it, and move its position back to where it was when it was initially
// captured.
//
void UWindowsViewport::SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL OnlyFocus )
{
	bWindowsMouseAvailable = !Capture;

	// If only handling focus windows, exit out.
	if( OnlyFocus )
		if( Window->hWnd != GetFocus() )
			return;

	// If capturing, windows requires clipping in order to keep focus.
	Clip |= Capture;

	// Get window rectangle.
	RECT TempRect;
	::GetClientRect( Window->hWnd, &TempRect );
	MapWindowPoints( Window->hWnd, NULL, (POINT*)&TempRect, 2 );

	// Handle capturing.
	if( Capture )
	{
		// Bring window to foreground.
		SetForegroundWindow(Window->hWnd);

		// Confine cursor to window.
		::GetCursorPos(&SavedCursor);
		SetCursorPos((TempRect.left + TempRect.right) / 2, (TempRect.top + TempRect.bottom) / 2);

		// Start capturing cursor.
		if (Window->hWnd)
			SetCapture(Window->hWnd);
		SystemParametersInfoX(SPI_SETMOUSE, 0, GetOuterUWindowsClient()->CaptureMouseInfo, 0);
		ShowCursor(FALSE);
	}
	else
	{
		// Release captured cursor.
		if (!(BlitFlags & BLIT_Fullscreen))
		{
			SetCapture(NULL);
			SystemParametersInfoX(SPI_SETMOUSE, 0, GetOuterUWindowsClient()->NormalMouseInfo, 0);
		}

		// Restore position.
		if (SavedCursor.x != -1)
		{
			SetCursorPos(SavedCursor.x, SavedCursor.y);
			SavedCursor.x = -1;
			while (ShowCursor(TRUE) < 0);
		}
	}

	// Handle clipping.
	ClipCursor( Clip ? &TempRect : NULL );
}

//
// Update input for viewport.
//
UBOOL UWindowsViewport::JoystickInputEvent( FLOAT Delta, EInputKey Key, FLOAT Scale, UBOOL DeadZone )
{
	Delta = (Delta-32768.f)/32768.f;
	if( DeadZone )
	{
		if( Delta > 0.2f )			Delta = (Delta - 0.2f) / 0.8f;
		else if( Delta < -0.2f )	Delta = (Delta + 0.2f) / 0.8f;
		else						Delta = 0.f;
	}
	return CauseInputEvent( Key, IST_Axis, Scale * Delta );
}

//
// Update input for this viewport.
//
void UWindowsViewport::UpdateInput( UBOOL Reset )
{
	BYTE Processed[256];
	appMemset( Processed, 0, 256 );
	//debugf(TEXT("%i"),(INT)GTempDouble);

	// Joystick.
	UWindowsClient* Client = GetOuterUWindowsClient();
	if( Client->JoyCaps.wNumButtons )
	{
		JOYINFOEX JoyInfo;
		appMemzero( &JoyInfo, sizeof(JoyInfo) );
		JoyInfo.dwSize  = sizeof(JoyInfo);
		JoyInfo.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNCENTERED | JOY_RETURNPOV | JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ;
		MMRESULT Result = joyGetPosEx( JOYSTICKID1, &JoyInfo );
		if( Result==JOYERR_NOERROR )
		{ 
			// Pass buttons to app.
			INT Index=0;
			for( Index=0; Index<16; Index++,JoyInfo.dwButtons/=2 )
			{
			   if( !Input->KeyDown(IK_Joy1+Index) && (JoyInfo.dwButtons & 1) )
				  CauseInputEvent( IK_Joy1+Index, IST_Press );
			   else if( Input->KeyDown(IK_Joy1+Index) && !(JoyInfo.dwButtons & 1) )
				  CauseInputEvent( IK_Joy1+Index, IST_Release );
				Processed[IK_Joy1+Index] = 1;
			}

			// Pass axes to app.
			JoystickInputEvent( JoyInfo.dwXpos, IK_JoyX, Client->ScaleXYZ, Client->DeadZoneXYZ );
			JoystickInputEvent( JoyInfo.dwYpos, IK_JoyY, Client->ScaleXYZ * (Client->InvertVertical ? 1.0 : -1.0), Client->DeadZoneXYZ );
			if( Client->JoyCaps.wCaps & JOYCAPS_HASZ )
				JoystickInputEvent( JoyInfo.dwZpos, IK_JoyZ, Client->ScaleXYZ, Client->DeadZoneXYZ );
			if( Client->JoyCaps.wCaps & JOYCAPS_HASR )
				JoystickInputEvent( JoyInfo.dwRpos, IK_JoyR, Client->ScaleRUV, Client->DeadZoneRUV );
			if( Client->JoyCaps.wCaps & JOYCAPS_HASU )
				JoystickInputEvent( JoyInfo.dwUpos, IK_JoyU, Client->ScaleRUV, Client->DeadZoneRUV );
			if( Client->JoyCaps.wCaps & JOYCAPS_HASV )
				JoystickInputEvent( JoyInfo.dwVpos, IK_JoyV, Client->ScaleRUV * (Client->InvertVertical ? 1.0 : -1.0), Client->DeadZoneRUV );
			if( Client->JoyCaps.wCaps & (JOYCAPS_POV4DIR|JOYCAPS_POVCTS) )
			{
				if( JoyInfo.dwPOV==JOY_POVFORWARD )
				{
					if( !Input->KeyDown(IK_JoyPovUp) )
						CauseInputEvent( IK_JoyPovUp, IST_Press );
					Processed[IK_JoyPovUp] = 1;
				}
				else if( JoyInfo.dwPOV==JOY_POVBACKWARD )
				{
					if( !Input->KeyDown(IK_JoyPovDown) )
						CauseInputEvent( IK_JoyPovDown, IST_Press );
					Processed[IK_JoyPovDown] = 1;
				}
				else if( JoyInfo.dwPOV==JOY_POVLEFT )
				{
					if( !Input->KeyDown(IK_JoyPovLeft) )
						CauseInputEvent( IK_JoyPovLeft, IST_Press );
					Processed[IK_JoyPovLeft] = 1;
				}
				else if( JoyInfo.dwPOV==JOY_POVRIGHT )
				{
					if( !Input->KeyDown(IK_JoyPovRight) )
						CauseInputEvent( IK_JoyPovRight, IST_Press );
					Processed[IK_JoyPovRight] = 1;
				}
			}
		}
		/*else
		{
			// Some joysticks sometimes fail reading.
			debugf( TEXT("Joystick read failed") );
			Client->JoyCaps.wNumButtons = 0;
		}*/
	}

	// Keyboard.
	Reset = Reset && GetFocus()==Window->hWnd;
	for( INT i=0; i<256; i++ )
	{
		if( !Processed[i] )
		{
			if( !Input->KeyDown(i) )
			{
				//old: if( Reset && (GetAsyncKeyState(i) & 0x8000) )
				if( Reset && (GetKeyState(i) & 0x8000) )
					CauseInputEvent( i, IST_Press );
			}
			else
			{
				//old: if( !(GetAsyncKeyState(i) & 0x8000) )
				if( !(GetKeyState(i) & 0x8000) )
					CauseInputEvent( i, IST_Release );
			}
		}
	}
}

void UWindowsViewport::AcquireDIMouse( UBOOL Capture )
{

}

/*-----------------------------------------------------------------------------
	Viewport WndProc.
-----------------------------------------------------------------------------*/

//
// Main viewport window function.
//
LONG UWindowsViewport::ViewportWndProc( UINT iMessage, WPARAM wParam, LPARAM lParam )
{
	UWindowsClient* Client = GetOuterUWindowsClient();
	if( HoldCount || Client->Viewports.FindItemIndex(this)==INDEX_NONE || !Actor )
		return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );

	// Statics.
	static UBOOL MovedSinceLeftClick=0;
	static UBOOL MovedSinceRightClick=0;
	static UBOOL MovedSinceMiddleClick=0;
	static DWORD StartTimeLeftClick=0;
	static DWORD StartTimeRightClick=0;
	static DWORD StartTimeMiddleClick=0;

	// Updates.
	if( iMessage==WindowMessageMouseWheel )
	{
		iMessage = WM_MOUSEWHEEL;
		wParam   = MAKEWPARAM(0,wParam);
	}

	// Message handler.
	switch( iMessage )
	{
		case WM_CREATE:
		{
         	// Set status.
			Status = WIN_ViewportNormal; 

			// Make this viewport current and update its title bar.
			GetOuterUClient()->MakeCurrent( this );

			return 0;
		}
		case WM_HOTKEY:
		{
			return 0;
		}
		case WM_DESTROY:
		{
			// If there's an existing Viewport structure corresponding to
			// this window, deactivate it.
			if( BlitFlags & BLIT_Fullscreen )
				EndFullscreen();

			// Free DIB section stuff (if any).
			if( hBitmap )
				DeleteObject( hBitmap );

			// Restore focus to caller if desired.
			DWORD ParentWindow=0;
			Parse( appCmdLine(), TEXT("HWND="), ParentWindow );
			if( ParentWindow )
			{
				::SetParent( Window->hWnd, NULL );
				SetFocus( (HWND)ParentWindow );

			}

			// Stop clipping.
			SetDrag( 0 );
			if( Status==WIN_ViewportNormal )
			{
				// Closed by user clicking on window's close button, so delete the viewport.
				Status = WIN_ViewportClosing; // Prevent recursion.
				delete this;
			}

			return 0;
		}
		case WM_PAINT:
		{
			if( BlitFlags & (BLIT_Fullscreen|BLIT_Direct3D|BLIT_HardwarePaint) )
			{
				//if( BlitFlags & BLIT_HardwarePaint )
					Repaint( 1 );
				ValidateRect( Window->hWnd, NULL );
				return 0;
			}
			else if( IsWindowVisible(Window->hWnd) && SizeX && SizeY && hBitmap )
			{
				PAINTSTRUCT ps;
				BeginPaint( Window->hWnd, &ps );
				HDC hDC = GetDC( Window->hWnd );
				if( hDC == NULL )
					appErrorf( TEXT("GetDC failed: %s"), appGetSystemErrorMessage() );
				if( SelectObject( Client->hMemScreenDC, hBitmap ) == NULL )
					appErrorf( TEXT("SelectObject failed: %s"), appGetSystemErrorMessage() );
				if( BitBlt( hDC, 0, 0, SizeX, SizeY, Client->hMemScreenDC, 0, 0, SRCCOPY ) == NULL )
					appErrorf( TEXT("BitBlt failed: %s"), appGetSystemErrorMessage() );
				if( ReleaseDC( Window->hWnd, hDC ) == NULL )
					appErrorf( TEXT("ReleaseDC failed: %s"), appGetSystemErrorMessage() );
				EndPaint( Window->hWnd, &ps );
				return 0;
			}
			else return 1;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			// Get key code.
			EInputKey Key = (EInputKey)wParam;

			// Send key to input system.
			if( Key==IK_Enter && (GetKeyState(VK_MENU)&0x8000) )
			{
				ToggleFullscreen();
			}
			else if( CauseInputEvent( Key, IST_Press ) )
			{	
				// Redraw if the viewport won't be redrawn on timer.
				if( !IsRealtime() )
					Repaint( 1 );
			}

			// Send to UnrealEd.
			if( ParentWindow && GIsEditor )
			{
				if( Key==IK_F1 )
					PostMessageX( ParentWindow, iMessage, IK_F2, lParam );
				else if( Key!=IK_Tab && Key!=IK_Enter && Key!=IK_Alt )
					PostMessageX( ParentWindow, iMessage, wParam, lParam );
			}

			// Set the cursor.
			if( GIsEditor )
				SetModeCursor();

			return 0;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			// Send to the input system.
			EInputKey Key = (EInputKey)wParam;
			if( CauseInputEvent( Key, IST_Release ) )
			{	
				// Redraw if the viewport won't be redrawn on timer.
				if( !IsRealtime() )
					Repaint( 1 );
			}

			// Pass keystroke on to UnrealEd.
			if( ParentWindow && GIsEditor )
			{				
				if( Key == IK_F1 )
					PostMessageX( ParentWindow, iMessage, IK_F2, lParam );
				else if( Key!=IK_Tab && Key!=IK_Enter && Key!=IK_Alt )
					PostMessageX( ParentWindow, iMessage, wParam, lParam );
			}
			if( GIsEditor )
				SetModeCursor();
			return 0;
		}
		case WM_SYSCHAR:
		case WM_CHAR:
		{
			EInputKey Key = (EInputKey)wParam;
			if( Key!=IK_Enter && Client->Engine->Key( this, Key ) )
			{
				// Redraw if needed.
				if( !IsRealtime() )
					Repaint( 1 );
				
				if( GIsEditor )
					SetModeCursor();
			}
			else if( iMessage == WM_SYSCHAR )
			{
				// Perform default processing.
				return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
			}
			return 0;
		}
		case WM_ERASEBKGND:
		{
			// Prevent Windows from repainting client background in white.
			return 0;
		}
		case WM_SETCURSOR:
		{
			if( (LOWORD(lParam)==1) || GIsSlowTask )
			{
				// In client area or processing slow task.
				if( GIsEditor )
					SetModeCursor();
				return 0;
			}
			else
			{
				// Out of client area.
				return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
			}
		}
		case WM_LBUTTONDBLCLK:
		{
			if( SizeX && SizeY && !(BlitFlags&BLIT_Fullscreen) )
			{
				Client->Engine->Click( this, MOUSE_LeftDouble, LOWORD(lParam), HIWORD(lParam) );
				if( !IsRealtime() )
					Repaint( 1 );
			}
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			if( Client->InMenuLoop )
				return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );

			// Doing this allows the editor to know where the mouse was last clicked in
			// world coordinates without having to do hit tests and such.  Useful for
			// a few things ...
			if( GIsEditor )
			{
				// Figure out where the mouse was clicked, in world coordinates.
				FVector V;
				FSceneNode* Frame = Client->Engine->Render->CreateMasterFrame( this, Actor->Location, Actor->Rotation, NULL );
				Client->Engine->Render->PreRender( Frame );
				Client->Engine->Render->Deproject( Frame, LOWORD(lParam), HIWORD(lParam), V );
				Client->Engine->Render->FinishMasterFrame();

				Client->Engine->edSetClickLocation(V);
			}

			// Send a notification message to the editor frame.  This of course relies on the 
			// window hierarchy not changing ... if it does, update this!
			HWND hwndEditorFrame = GetParent(GetParent(GetParent(ParentWindow)));
			SendMessageX( hwndEditorFrame, WM_COMMAND, WM_SETCURRENTVIEWPORT, (LPARAM)(DWORD)this );

			if( iMessage == WM_LBUTTONDOWN )
			{
				if( GIsEditor )
				{
					// Allows the editor to properly initiate box selections
					if( Input->KeyDown(IK_Ctrl) && Input->KeyDown(IK_Alt) )
						Client->Engine->Click( this, MOUSE_Left, LOWORD(lParam), HIWORD(lParam) );
				}
				MovedSinceLeftClick = 0;
				StartTimeLeftClick = GetMessageTime();
				CauseInputEvent( IK_LeftMouse, IST_Press );
			}
			else if( iMessage == WM_RBUTTONDOWN )
			{
				MovedSinceRightClick = 0;
				StartTimeRightClick = GetMessageTime();
				CauseInputEvent( IK_RightMouse, IST_Press );
			}
			else if( iMessage == WM_MBUTTONDOWN )
			{
				MovedSinceMiddleClick = 0;
				StartTimeMiddleClick = GetMessageTime();
				CauseInputEvent( IK_MiddleMouse, IST_Press );
			}
			SetDrag( 1 );
			return 0;
		}
		case WM_MOUSEACTIVATE:
		{
			// Activate this window and send the mouse-down message.
			return MA_ACTIVATE;
		}
		case WM_ACTIVATE:
		{

			// If window is becoming inactive, release the cursor.
			if( wParam==0 )
				SetDrag( 0 );

			return 0;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			// Exit if in menu loop.
			if( Client->InMenuLoop )
				return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );

			// Get mouse cursor position.
			POINT TempPoint={0,0};
			::ClientToScreen( Window->hWnd, &TempPoint );
			INT MouseX = SavedCursor.x!=-1 ? SavedCursor.x-TempPoint.x : LOWORD(lParam);
			INT MouseY = SavedCursor.x!=-1 ? SavedCursor.y-TempPoint.y : HIWORD(lParam);

			// Get time interval to determine if a click occured.
			INT DeltaTime, Button;
			EInputKey iKey;
			if( iMessage == WM_LBUTTONUP )
			{
				DeltaTime = GetMessageTime() - StartTimeLeftClick;
				iKey      = IK_LeftMouse;
				Button    = MOUSE_Left;
			}
			else if( iMessage == WM_MBUTTONUP )
			{
				DeltaTime = GetMessageTime() - StartTimeMiddleClick;
				iKey      = IK_MiddleMouse;
				Button    = MOUSE_Middle;
			}
			else
			{
				DeltaTime = GetMessageTime() - StartTimeRightClick;
				iKey      = IK_RightMouse;
				Button    = MOUSE_Right;
			}

			// Send to the input system.
			CauseInputEvent( iKey, IST_Release );

			// Release the cursor.
			if
			(	!Input->KeyDown(IK_LeftMouse)
			&&	!Input->KeyDown(IK_MiddleMouse)
			&&	!Input->KeyDown(IK_RightMouse) 
			&&	!(BlitFlags & BLIT_Fullscreen) )
                SetDrag( 0 );

			// Handle viewport clicking.
			if
			(	!(BlitFlags & BLIT_Fullscreen)
			&&	SizeX && SizeY 
			&&	!(MovedSinceLeftClick || MovedSinceRightClick || MovedSinceMiddleClick) )
			{
				Client->Engine->Click( this, Button, MouseX, MouseY );
				if( !IsRealtime() )
					Repaint( 1 );
			}

			// Update times.
			if		( iMessage == WM_LBUTTONUP ) MovedSinceLeftClick	= 0;
			else if	( iMessage == WM_RBUTTONUP ) MovedSinceRightClick	= 0;
			else if	( iMessage == WM_MBUTTONUP ) MovedSinceMiddleClick	= 0;

			return 0;
		}
		case WM_ENTERMENULOOP:
		{
			Client->InMenuLoop = 1;
			SetDrag( 0 );
			UpdateWindowFrame();
			return 0;
		}
		case WM_EXITMENULOOP:
		{
			Client->InMenuLoop = 0;
			return 0;
		}
		case WM_CANCELMODE:
		{
			SetDrag( 0 );
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			SWORD zDelta = HIWORD(wParam);
			if( zDelta )
			{
				CauseInputEvent( IK_MouseW, IST_Axis, zDelta );
				if( zDelta < 0 )
				{
					CauseInputEvent( IK_MouseWheelDown, IST_Press );
					CauseInputEvent( IK_MouseWheelDown, IST_Release );
				}
				else if( zDelta > 0 )
				{
					CauseInputEvent( IK_MouseWheelUp, IST_Press );
					CauseInputEvent( IK_MouseWheelUp, IST_Release );
				}
			}

			// Send to UnrealEd.
			if( ParentWindow && GIsEditor )
				PostMessageX( ParentWindow, iMessage, wParam, lParam );
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			//GTempDouble=GTempDouble+1;
			// If in a window, see if cursor has been captured; if not, ignore mouse movement.
			if (Client->InMenuLoop)
				break;

			// Get window rectangle.
			RECT TempRect;
			::GetClientRect(Window->hWnd, &TempRect);
			WORD Buttons = wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON);

			// If cursor isn't captured, just do MousePosition.
			if (!(BlitFlags & BLIT_Fullscreen) && SavedCursor.x == -1)
			{
				// Do mouse messaging.
				POINTS Point = MAKEPOINTS(lParam);
				DWORD ViewportButtonFlags = 0;
				if (Buttons & MK_LBUTTON) ViewportButtonFlags |= MOUSE_Left;
				if (Buttons & MK_RBUTTON) ViewportButtonFlags |= MOUSE_Right;
				if (Buttons & MK_MBUTTON) ViewportButtonFlags |= MOUSE_Middle;
				if (Input->KeyDown(IK_Shift)) ViewportButtonFlags |= MOUSE_Shift;
				if (Input->KeyDown(IK_Ctrl)) ViewportButtonFlags |= MOUSE_Ctrl;
				if (Input->KeyDown(IK_Alt)) ViewportButtonFlags |= MOUSE_Alt;
// jmarshall
				if (Input->KeyDown(IK_R)) ViewportButtonFlags |= MOUSE_R;
				if (Input->KeyDown(IK_T)) ViewportButtonFlags |= MOUSE_T;
				if (Input->KeyDown(IK_S)) ViewportButtonFlags |= MOUSE_S;
// jmarshall end
				Client->Engine->MousePosition(this, Buttons, Point.x - TempRect.left, Point.y - TempRect.top);
				if (bShowWindowsMouse && SelectedCursor >= 0 && SelectedCursor <= 6)
					SetCursor(StandardCursors[SelectedCursor]);
				break;
			}

			// Get center of window.			
			POINT TempPoint, Base;
			TempPoint.x = (TempRect.left + TempRect.right) / 2;
			TempPoint.y = (TempRect.top + TempRect.bottom) / 2;
			Base = TempPoint;

			// Movement accumulators.
			UBOOL Moved = 0;
			INT Cumulative = 0;

			// Grab all pending mouse movement.
			INT DX = 0, DY = 0;
		Loop:
			Buttons = wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON);
			POINTS Points = MAKEPOINTS(lParam);
			INT X = Points.x - Base.x;
			INT Y = Points.y - Base.y;
			Cumulative += Abs(X) + Abs(Y);
			DX += X;
			DY += Y;

			// Process valid movement.
			DWORD ViewportButtonFlags = 0;
			if (Buttons & MK_LBUTTON) ViewportButtonFlags |= MOUSE_Left;
			if (Buttons & MK_RBUTTON) ViewportButtonFlags |= MOUSE_Right;
			if (Buttons & MK_MBUTTON) ViewportButtonFlags |= MOUSE_Middle;
			if (Input->KeyDown(IK_Shift)) ViewportButtonFlags |= MOUSE_Shift;
			if (Input->KeyDown(IK_Ctrl)) ViewportButtonFlags |= MOUSE_Ctrl;
			if (Input->KeyDown(IK_Alt)) ViewportButtonFlags |= MOUSE_Alt;
// jmarshall
			if (Input->KeyDown(IK_R)) ViewportButtonFlags |= MOUSE_R;
			if (Input->KeyDown(IK_T)) ViewportButtonFlags |= MOUSE_T;
			if (Input->KeyDown(IK_S)) ViewportButtonFlags |= MOUSE_S;
// jmarshall end
			// Move viewport with buttons.
			if (X || Y)
			{
				Moved = 1;
				Client->Engine->MouseDelta(this, ViewportButtonFlags, X, Y);
			}

			// Handle any more mouse moves.
			MSG Msg;
			if (PeekMessageX(&Msg, Window->hWnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE))
			{
				lParam = Msg.lParam;
				wParam = Msg.wParam;
				Base.x = Points.x;
				Base.y = Points.y;
				goto Loop;
			}

			// Set moved-flags.
			if (Cumulative > 4)
			{
				if (wParam & MK_LBUTTON) MovedSinceLeftClick = 1;
				if (wParam & MK_RBUTTON) MovedSinceRightClick = 1;
				if (wParam & MK_MBUTTON) MovedSinceMiddleClick = 1;
			}

			// Send to input subsystem.
			if (DX) CauseInputEvent(IK_MouseX, IST_Axis, +DX);
			if (DY) CauseInputEvent(IK_MouseY, IST_Axis, -DY);

			// Put cursor back in middle of window.
			if (DX || DY)
			{
				::ClientToScreen(Window->hWnd, &TempPoint);
				SetCursorPos(TempPoint.x, TempPoint.y);
			}
			//else GTempDouble += 1000;

			// Viewport isn't realtime, so we must update the frame here and now.
			if (Moved && !IsRealtime())
			{
				if (Input->KeyDown(IK_Space))
					for (INT i = 0; i < Client->Viewports.Num(); i++)
						Client->Viewports(i)->Repaint(1);
				else
					Repaint(1);
			}

			// Dispatch keyboard events.
			while (PeekMessageX(&Msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
			{
				TranslateMessage(&Msg);
				DispatchMessageX(&Msg);
			}
			return 0;
		}
		case WM_SIZE:
		{
			INT NewX = LOWORD(lParam);
			INT NewY = HIWORD(lParam);
			if( BlitFlags & BLIT_Fullscreen )
			{
				// Window forced out of fullscreen.
				if( wParam==SIZE_RESTORED )
				{
					HoldCount++;
					Window->MoveWindow( SavedWindowRect, 1 );
					HoldCount--;
				}
				return 0;
			}
			else if( wParam==SIZE_RESTORED && DirectDrawMinimized )
			{
				DirectDrawMinimized = 0;
				ToggleFullscreen();
				return 0;
			}
			else
			{
				// Use resized window.
				if( RenDev && (BlitFlags & (BLIT_Direct3D)) )
				{
					RenDev->SetRes( NewX, NewY, ColorBytes, 0 );
				}
				else
				{
					ResizeViewport( BlitFlags|BLIT_NoWindowChange, NewX, NewY, ColorBytes );
				}
				if( GIsEditor )
					Repaint( 0 );
      			return 0;
        	}
		}
		case WM_KILLFOCUS:
		{
			SetMouseCapture( 0, 0, 0 );
			if( GIsRunning )
				Exec( TEXT("SETPAUSE 1"), *this );
			SetDrag( 0 );
			Input->ResetInput();
			if( BlitFlags & BLIT_Fullscreen )
			{
				debugf(TEXT("WM_KILLFOCUS"));
				EndFullscreen();
				HoldCount++;
				ShowWindow( Window->hWnd, SW_SHOWMINNOACTIVE );
				HoldCount--;
				DirectDrawMinimized = 1;
			}

			GetOuterUClient()->MakeCurrent( NULL );
			return 0;
		}
		case WM_SETFOCUS:
		{
			// Reset viewport's input.
			Exec( TEXT("SETPAUSE 0"), *this );
			Input->ResetInput();

			// Make this viewport current.
			GetOuterUClient()->MakeCurrent( this );
			SetModeCursor();
            return 0;
		}
		case WM_SYSCOMMAND:
		{
			DWORD nID = wParam & 0xFFF0;
			if( nID==SC_SCREENSAVE || nID==SC_MONITORPOWER )
			{
				// Return 1 to prevent screen saver.
				if( nID==SC_SCREENSAVE )
					debugf( NAME_Log, TEXT("Received SC_SCREENSAVE") );
				else
					debugf( NAME_Log, TEXT("Received SC_MONITORPOWER") );
				return 0;
			}
			else if( nID==SC_MAXIMIZE )
			{
				// Maximize.
				ToggleFullscreen();
				return 0;
			}
			else if
			(	(BlitFlags & BLIT_Fullscreen)
			&&	(nID==SC_NEXTWINDOW || nID==SC_PREVWINDOW || nID==SC_TASKLIST || nID==SC_HOTKEY) )
			{
				// Don't allow window changes here.
				return 0;
			}
			else return DefWindowProcX(Window->hWnd,iMessage,wParam,lParam);
		}
		case WM_POWER:
		{
			if( wParam )
			{
				if( wParam == PWR_SUSPENDREQUEST )
				{
					debugf( NAME_Log, TEXT("Received WM_POWER suspend") );

					// Prevent powerdown if dedicated server or using joystick.
					if( 1 )
						return PWR_FAIL;
					else
						return PWR_OK;
				}
				else
				{
					debugf( NAME_Log, TEXT("Received WM_POWER") );
					return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
				}
			}
			return 0;
		}
		case WM_DISPLAYCHANGE:
		{
			debugf( NAME_Log, TEXT("Viewport %s: WM_DisplayChange"), GetName() );
			return 0;
		}
		case WM_WININICHANGE:
		{
			if( !DeleteDC(Client->hMemScreenDC) )
				appErrorf( TEXT("DeleteDC failed: %s"), appGetSystemErrorMessage() );
			Client->hMemScreenDC = CreateCompatibleDC (NULL);
			return 0;
		}
		default:
		{
			return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
		}
	}
	return 0;
}
W_IMPLEMENT_CLASS(WWindowsViewportWindow)

/*-----------------------------------------------------------------------------
	DirectDraw support.
-----------------------------------------------------------------------------*/

//
// Set DirectDraw to a particular mode, with full error checking
// Returns 1 if success, 0 if failure.
//
UBOOL UWindowsViewport::ddSetMode( INT NewX, INT NewY, INT ColorBytes )
{
	UWindowsClient* Client = GetOuterUWindowsClient();
	check(Client->dd);
	HRESULT	Result;

	// Set the display mode.
	debugf( NAME_Log, TEXT("Setting %ix%ix%i"), NewX, NewY, ColorBytes*8 );
	Result = Client->dd->SetDisplayMode( NewX, NewY, ColorBytes*8, 0, 0 );
	if( Result!=DD_OK )
	{
		debugf( NAME_Log, TEXT("DirectDraw Failed %ix%ix%i: %s"), NewX, NewY, ColorBytes*8, ddError(Result) );
		Result = Client->dd->SetCooperativeLevel( NULL, DDSCL_NORMAL );
   		return 0;
	}

	// Create surfaces.
	DDSURFACEDESC SurfaceDesc;
	appMemzero( &SurfaceDesc, sizeof(DDSURFACEDESC) );
	SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
	SurfaceDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	SurfaceDesc.ddsCaps.dwCaps
	=	DDSCAPS_PRIMARYSURFACE
	|	DDSCAPS_FLIP
	|	DDSCAPS_COMPLEX
	|	(Client->SlowVideoBuffering ? DDSCAPS_SYSTEMMEMORY : DDSCAPS_VIDEOMEMORY);

	// Create the best possible surface for rendering.
	TCHAR* Descr=NULL;
	if( 1 )
	{
		// Try triple-buffered video memory surface.
		SurfaceDesc.dwBackBufferCount = 2;
		Result = Client->dd->CreateSurface( &SurfaceDesc, &ddFrontBuffer, NULL );
		Descr  = TEXT("Triple buffer");
	}
	if( Result != DD_OK )
   	{
		// Try to get a double buffered video memory surface.
		SurfaceDesc.dwBackBufferCount = 1; 
		Result = Client->dd->CreateSurface( &SurfaceDesc, &ddFrontBuffer, NULL );
		Descr  = TEXT("Double buffer");
    }
	if( Result != DD_OK )
	{
		// Settle for a main memory surface.
		SurfaceDesc.ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
		Result = Client->dd->CreateSurface( &SurfaceDesc, &ddFrontBuffer, NULL );
		Descr  = TEXT("System memory");
    }
	if( Result != DD_OK )
	{
		debugf( NAME_Log, TEXT("DirectDraw, no available modes %s"), ddError(Result) );
		Client->dd->RestoreDisplayMode();
		Client->dd->FlipToGDISurface();
	   	return 0;
	}
	debugf( NAME_Log, TEXT("DirectDraw: %s, %ix%i, Stride=%i"), Descr, NewX, NewY, SurfaceDesc.lPitch );
	debugf( NAME_Log, TEXT("DirectDraw: Rate=%i"), SurfaceDesc.dwRefreshRate );

	// Clear the screen.
	DDBLTFX ddbltfx;
	ddbltfx.dwSize = sizeof( ddbltfx );
	ddbltfx.dwFillColor = 0;
	ddFrontBuffer->Blt( NULL, NULL, NULL, DDBLT_COLORFILL, &ddbltfx );

	// Get a pointer to the back buffer.
	DDSCAPS caps;
	caps.dwCaps = DDSCAPS_BACKBUFFER;
	if( ddFrontBuffer->GetAttachedSurface( &caps, &ddBackBuffer )!=DD_OK )
	{
		debugf( NAME_Log, TEXT("DirectDraw GetAttachedSurface failed %s"), ddError(Result) );
		ddFrontBuffer->Release();
		ddFrontBuffer = NULL;
		Client->dd->RestoreDisplayMode();
		Client->dd->FlipToGDISurface();
		return 0;
	}

	// Get pixel format.
	DDPIXELFORMAT PixelFormat;
	PixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	Result = ddFrontBuffer->GetPixelFormat( &PixelFormat );
	if( Result!=DD_OK )
	{
		debugf( TEXT("DirectDraw GetPixelFormat failed: %s"), ddError(Result) );
		ddBackBuffer->Release();
		ddBackBuffer = NULL;
		ddFrontBuffer->Release();
		ddFrontBuffer = NULL;
		Client->dd->RestoreDisplayMode();
		Client->dd->FlipToGDISurface();
		return 0;
	}

	// See if we're in a 16-bit color mode.
	Caps &= ~CC_RGB565;
	if( ColorBytes==2 && PixelFormat.dwRBitMask==0xf800 ) 
		Caps |= CC_RGB565;

	// Flush the cache.
	GCache.Flush();

	// Success.
	return 1;
}

/*-----------------------------------------------------------------------------
	DirectInput support.
-----------------------------------------------------------------------------*/
UBOOL UWindowsViewport::diSetupKeyboardMouse()
{
	// Set baseline.
	diOldMouseX = 0;
	diOldMouseY = 0;
	diOldMouseZ = 0;
	diLMouseDown = 0;
	diRMouseDown = 0;
	diMMouseDown = 0;

	return true;
}

void UWindowsViewport::diShutdownKeyboardMouse()
{

}


/*-----------------------------------------------------------------------------
	Lock and Unlock.
-----------------------------------------------------------------------------*/

//
// Lock the viewport window and set the approprite Screen and RealScreen fields
// of Viewport.  Returns 1 if locked successfully, 0 if failed.  Note that a
// lock failing is not a critical error; it's a sign that a DirectDraw mode
// has ended or the user has closed a viewport window.
//
//UBOOL UWindowsViewport::Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )
UBOOL UWindowsViewport::Lock( FColor FogColor, float FogDensity, INT FogDistance, FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )
{
	UWindowsClient* Client = GetOuterUWindowsClient();
	clock(Client->DrawCycles);

	// Make sure window is lockable.
	if( (Window->hWnd && !IsWindow(Window->hWnd)) || HoldCount || !SizeX || !SizeY || !RenDev )
      	return 0;

	// Get info.
	Stride = SizeX;
	if( BlitFlags & BLIT_DirectDraw )
	{
		// Lock DirectDraw.
		check(!(BlitFlags&BLIT_DibSection));
		HRESULT Result;
  		if( ddFrontBuffer->IsLost() == DDERR_SURFACELOST )
		{
			Result = ddFrontBuffer->Restore();
   			if( Result != DD_OK )
			{
				debugf( NAME_Log, TEXT("DirectDraw Lock Restore failed %s"), ddError(Result) );
				ResizeViewport( BLIT_DibSection );//!!failure of d3d?
				return 0;
			}
		}
		appMemzero( &ddSurfaceDesc, sizeof(ddSurfaceDesc) );
  		ddSurfaceDesc.dwSize = sizeof(ddSurfaceDesc);
		Result = ddBackBuffer->Lock( NULL, &ddSurfaceDesc, DDLOCK_WAIT|DD_OTHERLOCKFLAGS, NULL );
  		if( Result != DD_OK )
		{
			debugf( NAME_Log, TEXT("DirectDraw Lock failed: %s"), ddError(Result) );
  			return 0;
		}
		if( ddSurfaceDesc.lPitch )
			Stride = ddSurfaceDesc.lPitch/ColorBytes;
		ScreenPointer = (BYTE*)ddSurfaceDesc.lpSurface;
		check(ScreenPointer);
	}
	else if( BlitFlags & BLIT_DibSection )
	{
		check(!(BlitFlags&BLIT_DirectDraw));
		check(ScreenPointer);
	}

	// Success here, so pass to superclass.
	unclock(Client->DrawCycles);
	return UViewport::Lock(FogColor,FogDensity,FogDistance,FlashScale,FlashFog,ScreenClear,RenderLockFlags,HitData,HitSize);
}

//
// Unlock the viewport window.  If Blit=1, blits the viewport's frame buffer.
//
void UWindowsViewport::Unlock( UBOOL Blit )
{
	UWindowsClient* Client = GetOuterUWindowsClient();
	check(!HoldCount);
	Client->DrawCycles=0;
	clock(Client->DrawCycles);
	UViewport::Unlock( Blit );
	if( BlitFlags & BLIT_DirectDraw )
	{
		// Handle DirectDraw.
		HRESULT Result;
		Result = ddBackBuffer->Unlock( ddSurfaceDesc.lpSurface );
		if( Result ) 
		 	appErrorf( TEXT("DirectDraw Unlock: %s"), ddError(Result) );
		if( Blit )
		{
			HRESULT Result = ddFrontBuffer->Flip( NULL, DDFLIP_WAIT );
			if( Result != DD_OK )
				appErrorf( TEXT("DirectDraw Flip failed: %s"), ddError(Result) );
		}
	}
	else if( BlitFlags & BLIT_DibSection )
	{
		// Handle CreateDIBSection.
		if( Blit )
		{
			HDC hDC = GetDC( Window->hWnd );
			if( hDC == NULL )
				appErrorf( TEXT("GetDC failed: %s"), appGetSystemErrorMessage() );
			if( SelectObject( Client->hMemScreenDC, hBitmap ) == NULL )
				appErrorf( TEXT("SelectObject failed: %s"), appGetSystemErrorMessage() );
			if( BitBlt( hDC, 0, 0, SizeX, SizeY, Client->hMemScreenDC, 0, 0, SRCCOPY ) == NULL )
				appErrorf( TEXT("BitBlt failed: %s"), appGetSystemErrorMessage() );
			if( ReleaseDC( Window->hWnd, hDC ) == NULL )
				appErrorf( TEXT("ReleaseDC failed: %s"), appGetSystemErrorMessage() );
		}
	}
	unclock(Client->DrawCycles);
}

/*-----------------------------------------------------------------------------
	Viewport modes.
-----------------------------------------------------------------------------*/

//
// Try switching to a new rendering device.
//
void UWindowsViewport::TryRenderDevice( const TCHAR* ClassName, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
{
	// Shut down current rendering device.
	if( RenDev )
	{
		RenDev->Exit();
		delete RenDev; RenDev = NULL;
	}

	// Use appropriate defaults.
	UWindowsClient* C = GetOuterUWindowsClient();
	if( NewX==INDEX_NONE )			NewX = Fullscreen ? C->FullscreenViewportX : C->WindowedViewportX;
	if( NewY==INDEX_NONE )			NewY = Fullscreen ? C->FullscreenViewportY : C->WindowedViewportY;
	
	if( NewColorBytes==INDEX_NONE )	NewColorBytes = Fullscreen ? C->FullscreenColorBits/8 : ColorBytes;

	
	// Find device driver.
	UClass* RenderClass = UObject::StaticLoadClass( URenderDevice::StaticClass(), NULL, ClassName, NULL, 0, NULL );
	if( RenderClass )
	{
		HoldCount++;
		RenDev = ConstructObject<URenderDevice>( RenderClass, this );
		if( RenDev->Init( this, NewX, NewY, NewColorBytes, Fullscreen ) )
		{
			if( GIsRunning )
				Actor->GetLevel()->DetailChange( RenDev->HighDetailActors );
		}
		else
		{
			debugf( NAME_Log, LocalizeError("Failed3D") );
			if(RenDev) { delete RenDev; RenDev=NULL; }
			RenDev = NULL;
		}
		HoldCount--;
	}
	GRenderDevice = RenDev;
}

// If in fullscreen mode, end it and return to Windows.
void UWindowsViewport::EndFullscreen()
{
	UWindowsClient* Client = GetOuterUWindowsClient();
	debugf(TEXT("EndFullscreen"));
	if( RenDev && (BlitFlags & BLIT_Direct3D))
	{
		RenDev->SetRes( Client->WindowedViewportX, Client->WindowedViewportY, ColorBytes, 0 );
	}
	else
	{
		ResizeViewport( BLIT_DibSection );
	}
	UpdateWindowFrame();
}

// Toggle fullscreen.
void UWindowsViewport::ToggleFullscreen()
{
	if( BlitFlags & BLIT_Fullscreen )
	{
		EndFullscreen();
	}
	else if( !(Actor->ShowFlags & SHOW_ChildWindow) )
	{
		debugf(TEXT("AttemptFullscreen"));
		TryRenderDevice( TEXT("ini:Engine.Engine.GameRenderDevice"), INDEX_NONE, INDEX_NONE, INDEX_NONE, 1 );
		if( !RenDev )
			TryRenderDevice( TEXT("ini:Engine.Engine.WindowedRenderDevice"), INDEX_NONE, INDEX_NONE, INDEX_NONE, 1 );
		if( !RenDev )
			TryRenderDevice( TEXT("ini:Engine.Engine.WindowedRenderDevice"), INDEX_NONE, INDEX_NONE, INDEX_NONE, 0 );
	}
}

// Resize the viewport.
UBOOL UWindowsViewport::ResizeViewport( DWORD NewBlitFlags, INT InNewX, INT InNewY, INT InNewColorBytes )
{
	UWindowsClient* Client = GetOuterUWindowsClient();

	// Handle temporary viewports.
	// Andrew says: I made this keep the BLIT_Temporary flag so the temporary screen buffer doesn't get leaked
	// during light rebuilds.
	if( BlitFlags & BLIT_Temporary )
		NewBlitFlags = (NewBlitFlags & ~(BLIT_DirectDraw | BLIT_DibSection)) | BLIT_Temporary;

	// Handle DirectDraw not available.
	if( (NewBlitFlags & BLIT_DirectDraw) && !Client->dd )
		NewBlitFlags = (NewBlitFlags | BLIT_DibSection) & ~(BLIT_Fullscreen | BLIT_DirectDraw);

	// Remember viewport.
	UViewport* SavedViewport = NULL;
	if( Client->Engine->Audio && !GIsEditor && !(GetFlags() & RF_Destroyed) )
		SavedViewport = Client->Engine->Audio->GetViewport();

	// Accept default parameters.
	INT NewX          = InNewX         ==INDEX_NONE ? SizeX      : InNewX;
	INT NewY          = InNewY         ==INDEX_NONE ? SizeY      : InNewY;
	INT NewColorBytes = InNewColorBytes==INDEX_NONE ? ColorBytes : InNewColorBytes;

	// Shut down current frame.
	if( BlitFlags & BLIT_DirectDraw )
	{
		debugf( NAME_Log, TEXT("DirectDraw session ending") );
		if( SavedViewport )
			Client->Engine->Audio->SetViewport( NULL );
		check(ddBackBuffer);
		ddBackBuffer->Release();
		check(ddFrontBuffer);
		ddFrontBuffer->Release();
		if( !(NewBlitFlags & BLIT_DirectDraw) )
		{
			HoldCount++;
			Client->ddEndMode();
			HoldCount--;
		}
	}
	else if( BlitFlags & BLIT_DibSection )
	{
		if( hBitmap )
			DeleteObject( hBitmap );
		hBitmap = NULL;
	}

	// Get this window rect.
	FRect WindowRect = SavedWindowRect;
	if( Window->hWnd && !(BlitFlags & BLIT_Fullscreen) && !(NewBlitFlags&BLIT_NoWindowChange) )
		WindowRect = Window->GetWindowRect();

	// Default resolution handling.
	NewX = InNewX!=INDEX_NONE ? InNewX : (NewBlitFlags&BLIT_Fullscreen) ? Client->FullscreenViewportX : Client->WindowedViewportX;
	NewY = InNewX!=INDEX_NONE ? InNewY : (NewBlitFlags&BLIT_Fullscreen) ? Client->FullscreenViewportY : Client->WindowedViewportY;

	// Align NewX.
	check((NewX>=0)&&(NewY>=0));
	NewX=Align(NewX,2);

	// If currently fullscreen, end it.
	if( BlitFlags & BLIT_Fullscreen )
	{
		// Saved parameters.
		SetFocus( Window->hWnd );
		if( InNewColorBytes==INDEX_NONE )
			NewColorBytes = SavedColorBytes;

		// Remember saved info.
		WindowRect          = SavedWindowRect;
		Caps                = SavedCaps;

		// Restore window topness.
		SetTopness();
		SetDrag( 0 );

		// Stop inhibiting windows keys.
		UnregisterHotKey( Window->hWnd, Client->hkAltEsc  );
		UnregisterHotKey( Window->hWnd, Client->hkAltTab  );
		UnregisterHotKey( Window->hWnd, Client->hkCtrlEsc );
		UnregisterHotKey( Window->hWnd, Client->hkCtrlTab );
		//DWORD Old=0;
		//SystemParametersInfoX( SPI_SCREENSAVERRUNNING, 0, &Old, 0 );
	}

	// If transitioning into fullscreen.
	if( (NewBlitFlags & BLIT_Fullscreen) && !(BlitFlags & BLIT_Fullscreen) )
	{
		// Save window parameters.
		SavedWindowRect = WindowRect;
		SavedColorBytes	= ColorBytes;
		SavedCaps       = Caps;

		// Make "Advanced Options" not return fullscreen after this.
		if( Client->ConfigProperties )
		{
			Client->ConfigReturnFullscreen = 0;
			DestroyWindow( *Client->ConfigProperties );
		}

		// Turn off window border and menu.
		HoldCount++;
		SendMessageX( Window->hWnd, WM_SETREDRAW, 0, 0 );
		SetMenu( Window->hWnd, NULL );
		if( !GIsEditor )
		{
			SetWindowLongX( Window->hWnd, GWL_STYLE, GetWindowLongX(Window->hWnd,GWL_STYLE) & ~(WS_CAPTION|WS_THICKFRAME) );
			Borderless = 1;
		}
		SendMessageX( Window->hWnd, WM_SETREDRAW, 1, 0 );
		HoldCount--;
	}

	// Handle display method.
	if( NewBlitFlags & BLIT_DirectDraw )
	{
		// Go into closest matching DirectDraw mode.
		INT BestMode=-1, BestDelta=MAXINT;		
		for( INT i=0; i<Client->DirectDrawModes[ColorBytes].Num(); i++ )
		{
			INT Delta = Abs(Client->DirectDrawModes[ColorBytes](i).X-NewX) + Abs(Client->DirectDrawModes[ColorBytes](i).Y-NewY);
			if( Delta < BestDelta )
			{
				BestMode  = i;
				BestDelta = Delta;
			}
		}
		if( BestMode>=0 )
		{
			// Try to go into DirectDraw.
			NewX = Client->DirectDrawModes[ColorBytes](BestMode).X;
			NewY = Client->DirectDrawModes[ColorBytes](BestMode).Y;
			HoldCount++;
			if( !(BlitFlags & BLIT_DirectDraw) )
			{
				if( SavedViewport )
					Client->Engine->Audio->SetViewport( NULL );
				HRESULT Result = Client->dd->SetCooperativeLevel( Window->hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT );
				if( Result != DD_OK )
				{
					debugf( TEXT("DirectDraw SetCooperativeLevel failed: %s"), ddError(Result) );
   					return 0;
				}
			}
			SetCursor( NULL );
			UBOOL Result = ddSetMode( NewX, NewY, NewColorBytes );
			SetForegroundWindow( Window->hWnd );
			HoldCount--;
			if( !Result )
			{
				// DirectDraw failed.
				HoldCount++;
				Client->dd->SetCooperativeLevel( NULL, DDSCL_NORMAL );
				Window->MoveWindow( SavedWindowRect, 1 );
				SetTopness();
				HoldCount--;
				debugf( LocalizeError("DDrawMode") );
				return 0;
			}
		}
		else
		{
			debugf( TEXT("No DirectDraw modes") );
			return 0;
		}
	}
	else if( (NewBlitFlags&BLIT_DibSection) && NewX && NewY )
	{
		// Create DIB section.
		struct { BITMAPINFOHEADER Header; RGBQUAD Colors[256]; } Bitmap;

		// Init BitmapHeader for DIB.
		appMemzero( &Bitmap, sizeof(Bitmap) );
		Bitmap.Header.biSize			= sizeof(BITMAPINFOHEADER);
		Bitmap.Header.biWidth			= NewX;
		Bitmap.Header.biHeight			= -NewY;
		Bitmap.Header.biPlanes			= 1;
		Bitmap.Header.biBitCount		= NewColorBytes * 8;
		Bitmap.Header.biSizeImage		= NewX * NewY * NewColorBytes;

		// Handle color depth.
		if( NewColorBytes==2 )
		{
			// 16-bit color (565).
			Bitmap.Header.biCompression = BI_BITFIELDS;
			*(DWORD *)&Bitmap.Colors[0] = (Caps & CC_RGB565) ? 0xF800 : 0x7C00;
			*(DWORD *)&Bitmap.Colors[1] = (Caps & CC_RGB565) ? 0x07E0 : 0x03E0;
			*(DWORD *)&Bitmap.Colors[2] = (Caps & CC_RGB565) ? 0x001F : 0x001F;
		}
		else if( NewColorBytes==3 || NewColorBytes==4 )
		{
			// 24-bit or 32-bit color.
			Bitmap.Header.biCompression = BI_RGB;
			*(DWORD *)&Bitmap.Colors[0] = 0;
		}
		else appErrorf( TEXT("Invalid DibSection color depth %i"), NewColorBytes );

		// Create DIB section.
		HDC TempDC = GetDC(0);
		check(TempDC);
		hBitmap = CreateDIBSection( TempDC, (BITMAPINFO*)&Bitmap.Header, DIB_RGB_COLORS, (void**)&ScreenPointer, NULL, 0 );
		ReleaseDC( 0, TempDC );
		if( !hBitmap )
			appErrorf( LocalizeError("OutOfMemory",TEXT("Core")) );
		check(ScreenPointer);
	}
	else if( !(NewBlitFlags & BLIT_Temporary) )
	{
		ScreenPointer = NULL;
	}

	// OpenGL handling.
	if( (NewBlitFlags & BLIT_Fullscreen) && !GIsEditor && RenDev && appStricmp(RenDev->GetClass()->GetName(),TEXT("OpenGLRenderDevice"))==0 )
	{
		// Turn off window border and menu.
		HoldCount++;
		SendMessageX( Window->hWnd, WM_SETREDRAW, 0, 0 );
		Window->MoveWindow( FRect(0,0,NewX,NewY), 0 );
		SendMessageX( Window->hWnd, WM_SETREDRAW, 1, 0 );
		HoldCount--;
	}

	// Set new info.
	DWORD OldBlitFlags = BlitFlags;
	BlitFlags          = NewBlitFlags & ~BLIT_ParameterFlags;
	SizeX              = NewX;
	SizeY              = NewY;
	ColorBytes         = NewColorBytes ? NewColorBytes : ColorBytes;

	// If transitioning out of fullscreen.
	if( !(NewBlitFlags & BLIT_Fullscreen) && (OldBlitFlags & BLIT_Fullscreen) )
	{
		SetMouseCapture( 0, 0, 0 );
	}

	// Handle type.
	if( NewBlitFlags & BLIT_Fullscreen )
	{	
		// Handle fullscreen input.
		SetDrag( 1 );
		SetMouseCapture( 1, 1, 0 );
		RegisterHotKey( Window->hWnd, Client->hkAltEsc,  MOD_ALT,     VK_ESCAPE );
		RegisterHotKey( Window->hWnd, Client->hkAltTab,  MOD_ALT,     VK_TAB    );
		RegisterHotKey( Window->hWnd, Client->hkCtrlEsc, MOD_CONTROL, VK_ESCAPE );
		RegisterHotKey( Window->hWnd, Client->hkCtrlTab, MOD_CONTROL, VK_TAB    );
		//DWORD Old=0;
		//SystemParametersInfoX( SPI_SCREENSAVERRUNNING, 1, &Old, 0 );
	}
	else if( !(NewBlitFlags & BLIT_Temporary) && !(NewBlitFlags & BLIT_NoWindowChange) )
	{
		// Turn on window border and menu.
		if( Borderless )
		{
			HoldCount++;
			SetWindowLongX( Window->hWnd, GWL_STYLE, GetWindowLongX(Window->hWnd,GWL_STYLE) | (WS_CAPTION|WS_THICKFRAME) );
			HoldCount--;
		}

		// Going to a window.
		FRect ClientRect(0,0,NewX,NewY);
		AdjustWindowRect( ClientRect, GetWindowLongX(Window->hWnd,GWL_STYLE), (Actor->ShowFlags & SHOW_Menu)!=0 );

		// Resize the window and repaint it.
		if( !(Actor->ShowFlags & SHOW_ChildWindow) )
		{
			HoldCount++;
			Window->MoveWindow( FRect(WindowRect.Min,WindowRect.Min+ClientRect.Size()), 1 );
			HoldCount--;
		}
		SetDrag( 0 );
	}

	// Update audio.
	if( SavedViewport && SavedViewport!=Client->Engine->Audio->GetViewport() )
		Client->Engine->Audio->SetViewport( SavedViewport );

	// Update the window.
	UpdateWindowFrame();

	// Save info.
	if( RenDev && !GIsEditor )
	{
		if( NewBlitFlags & BLIT_Fullscreen )
		{
			if( NewX && NewY )
			{
				Client->FullscreenViewportX  = NewX;
				Client->FullscreenViewportY  = NewY;
				Client->FullscreenColorBits  = NewColorBytes*8;
			}
		}
		else
		{
			if( NewX && NewY )
			{
				Client->WindowedViewportX  = NewX;
				Client->WindowedViewportY  = NewY;
				Client->WindowedColorBits  = NewColorBytes*8;
			}
		}
		Client->SaveConfig();
	}

	return 1;
}
