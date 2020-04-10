///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: November 29th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#include "Game/Framework/App.h"
#include "Game/Framework/Game.h"
#include "Game/Framework/GameCommon.h"
#include "Engine/Event/EventSystem.h"
#include "Engine/Framework/EngineCommon.h"
#include "Engine/Framework/Window.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Job/JobSystem.h"
#include "Engine/Render/Core/DX11Common.h"
#include "Engine/Render/Core/RenderContext.h"
#include "Engine/Time/Clock.h"
#include "Engine/Utility/StringID.h"

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------
App* g_app = nullptr;

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
bool AppMessageHandler(unsigned int msg, size_t wParam, size_t lParam)
{
	UNUSED(wParam);
	UNUSED(lParam);

	switch (msg)
	{
	case WM_CLOSE: // App close requested via "X" button, "Close Window" on task bar, or "Close" from system menu, or Alt-F4
	{
		g_app->Quit();
		return true;
	}
	case WM_KEYDOWN:
	{
		unsigned char asKey = (unsigned char)wParam;
		if (asKey == VK_ESCAPE)
		{
			g_app->Quit();
			return true;
		}
		break;
	}
	}

	return false;
}


//-------------------------------------------------------------------------------------------------
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
		DispatchMessage(&queuedMessage); // This tells Windows to call the "WindowsMessageHandlingProcedure" above
	}
}


///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS IMPLEMENTATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
App::App()
{
}


//-------------------------------------------------------------------------------------------------
App::~App()
{
}


//-------------------------------------------------------------------------------------------------
void App::Initialize()
{
	g_app = new App();

	EventSystem::Initialize();
	Window::Initialize((21.f / 9.f), "Engine Test - MechroEngine");
	g_window->RegisterMessageHandler(AppMessageHandler);
	Clock::ResetMaster();
	RenderContext::Initialize();
	InputSystem::Initialize();
	DebugSIDSystem::Initialize();
	JobSystem::Initialize();

	g_app->m_game = new Game();
}


//-------------------------------------------------------------------------------------------------
void App::Shutdown()
{
	SAFE_DELETE_POINTER(g_app->m_game);

	JobSystem::Shutdown();
	DebugSIDSystem::Shutdown();
	InputSystem::Shutdown();
	RenderContext::Shutdown();
	Window::Shutdown();
	EventSystem::Shutdown();

	SAFE_DELETE_POINTER(g_app);
}


//-------------------------------------------------------------------------------------------------
void App::RunFrame()
{
	Clock::BeginMasterFrame();
	RunMessagePump();

	// Begin Frames...
	g_renderContext->BeginFrame();
	g_inputSystem->BeginFrame();

	// Game Frame
	ProcessInput();
	Update();
	Render();

	// End Frames...
	g_renderContext->EndFrame();
	Sleep(15);
}


//-------------------------------------------------------------------------------------------------
void App::Quit()
{
	m_isQuitting = true;
}


//-------------------------------------------------------------------------------------------------
void App::ProcessInput()
{

}


//-------------------------------------------------------------------------------------------------
void App::Update()
{
	m_game->Update();
}


//-------------------------------------------------------------------------------------------------
void App::Render()
{
	m_game->Render();
}
