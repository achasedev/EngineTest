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
#include "Engine/Render/Core/DX11Common.h"
#include "Engine/Render/Core/RenderContext.h"
#include "Engine/Framework/EngineCommon.h"
#include "Engine/Framework/Window.h"
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
App* App::s_instance = nullptr;

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
bool AppMessageHandler(unsigned int msg, size_t wParam, size_t lParam)
{
	UNUSED(lParam);

	switch (msg)
	{
	case WM_CLOSE: // App close requested via "X" button, "Close Window" on task bar, or "Close" from system menu, or Alt-F4
	{
		App::GetInstance()->Quit();
		return true;
	}
	case WM_KEYDOWN: // TODO: Remove
	{
		unsigned char asKey = (unsigned char)wParam;
		if (asKey == VK_ESCAPE)
		{
			App::GetInstance()->Quit();
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
	s_instance = new App();

	Window::Initialize((21.f / 9.f), "Engine Test - MechroEngine");
	Window::GetInstance()->RegisterMessageHandler(AppMessageHandler);
	RenderContext::Initialize();
	StringIDManager::Initialize();

	s_instance->m_game = new Game();
}


//-------------------------------------------------------------------------------------------------
void App::Shutdown()
{
	SAFE_DELETE_POINTER(s_instance->m_game);

	StringIDManager::Shutdown();
	RenderContext::Shutdown();
	Window::ShutDown();

	delete s_instance;
	s_instance = nullptr;
}


//-------------------------------------------------------------------------------------------------
void App::RunFrame()
{
	RunMessagePump();

	// Begin Frames...
	RenderContext::GetInstance()->BeginFrame();

	ProcessInput();
	Update();
	Render();

	// End Frames...
	RenderContext::GetInstance()->EndFrame();
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
