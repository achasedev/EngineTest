#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in very few places
//#include <crtdbg.h>
#include "Engine/Framework/Window.h"

//-----------------------------------------------------------------------------------------------
// #SD1ToDo: Move this macro later to a more central place, e.g. Engine/Core/EngineCommon.hpp
//
#define UNUSED(x) (void)(x);

//-----------------------------------------------------------------------------------------------
// #SD1ToDo: Move each of these items to its proper place, once that place is established
// 
bool g_isQuitting = false;						// ...becomes App::m_isQuitting
Window* g_window = nullptr;


//-----------------------------------------------------------------------------------------------
// Processes all Windows messages (WM_xxx) for this app that have queued up since last frame.
// For each message in the queue, our WindowsMessageHandlingProcedure (or "WinProc") function
//	is called, telling us what happened (key up/down, minimized/restored, gained/lost focus, etc.)
//
void RunMessagePump()
{
	MSG queuedMessage;
	for (;;)
	{
		const BOOL wasMessagePresent = PeekMessage(&queuedMessage, NULL, 0, 0, PM_REMOVE);
		if (!wasMessagePresent)
		{
			break;
		}

		TranslateMessage(&queuedMessage);
		DispatchMessage(&queuedMessage); // This tells Windows to call our "WindowsMessageHandlingProcedure" function
	}
}


//-----------------------------------------------------------------------------------------------
// #SD1ToDo: Remove this function and replace it with TheApp::Update()
//
void Update()
{
}




//-----------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
//
void RunFrame()
{
	RunMessagePump();
	Update();
}


//-----------------------------------------------------------------------------------------------
void Initialize()
{
	g_window = new Window();
	g_window->Initialize();
}


//-----------------------------------------------------------------------------------------------
void Shutdown()
{
	delete g_window;
	g_window = nullptr;
}


//-----------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int)
{
	UNUSED(commandLineString);
	UNUSED(applicationInstanceHandle);

	Initialize();

	// Program main loop; keep running frames until it's time to quit
	while (!g_isQuitting) // #SD1ToDo: ...becomes:  !g_theApp->IsQuitting()
	{
		RunFrame();
	}

	Shutdown();
	return 0;
}


