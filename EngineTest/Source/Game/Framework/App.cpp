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
#include "Game/Framework/GameCommands.h"
#include "Game/Framework/GameCommon.h"
#include "Engine/Event/EventSystem.h"
#include "Engine/Core/ConsoleCommand.h"
#include "Engine/Core/DevConsole.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/Window.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Job/JobSystem.h"
#include "Engine/Render/DX11Common.h"
#include "Engine/Render/RenderContext.h"
#include "Engine/Render/Debug/DebugRenderSystem.h"
#include "Engine/Resource/ResourceSystem.h"
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

	StringIdSystem::Initialize();
	EventSystem::Initialize();
	Window::Initialize((21.f / 9.f), "Hello");
	g_window->RegisterMessageHandler(AppMessageHandler);
	Clock::ResetMaster();
	RenderContext::Initialize();
	InputSystem::Initialize();
	JobSystem::Initialize();
	ResourceSystem::Initialize();
	DevConsole::Initialize();
	DebugRenderSystem::Initialize();

	g_game = new Game();
	g_app->RegisterGameCommands();
}


//-------------------------------------------------------------------------------------------------
void App::Shutdown()
{
	SAFE_DELETE(g_game);

	DebugRenderSystem::Shutdown();
	ResourceSystem::Shutdown();
	DevConsole::Shutdown();
	JobSystem::Shutdown();
	InputSystem::Shutdown();
	RenderContext::Shutdown();
	g_window->UnregisterMessageHandler(AppMessageHandler);
	Window::Shutdown();
	EventSystem::Shutdown();
	StringIdSystem::Shutdown();

	SAFE_DELETE(g_app);
}


//-------------------------------------------------------------------------------------------------
void App::RunFrame()
{
	Clock::BeginMasterFrame();
	RunMessagePump();

	// Begin Frames...
	g_inputSystem->BeginFrame();
	g_renderContext->BeginFrame();
	g_devConsole->BeginFrame();
	g_eventSystem->BeginFrame();

	// Game Frame
	ProcessInput();
	Update();
	Render();

	// End Frames...
	g_devConsole->EndFrame();
	g_renderContext->EndFrame();
	g_inputSystem->EndFrame();
}


//-------------------------------------------------------------------------------------------------
void App::Quit()
{
	m_isQuitting = true;
}


//-------------------------------------------------------------------------------------------------
void App::ProcessInput()
{
	if (g_devConsole->IsActive())
	{
		g_devConsole->ProcessInput();
	}
	else
	{
		g_game->ProcessInput();
	}
}


//-------------------------------------------------------------------------------------------------
void App::Update()
{
	g_game->Update();
	g_devConsole->Update();
}


//-------------------------------------------------------------------------------------------------
void App::Render()
{
	g_game->Render();
	g_debugRenderSystem->Render();
	g_devConsole->Render();
}


//-------------------------------------------------------------------------------------------------
void App::RegisterGameCommands()
{
	ConsoleCommand::Register(SID("exit"), "Shuts down the program", "exit (NO_PARAMS)", Command_Exit, false);
	ConsoleCommand::Register(SID("collision_debug"), "Sets the collision debug draw flags", "collision_debug <collider - contact - bounds - leaf_bounds>", Command_SetCollisionDebugFlags, false);
}
