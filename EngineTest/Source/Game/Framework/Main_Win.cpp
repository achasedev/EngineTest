#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Game/Framework/App.h"
#include "Game/Framework/GameCommon.h"
#include "Engine/Core/EngineCommon.h"

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int)
{
	UNUSED(commandLineString);
	UNUSED(applicationInstanceHandle);

	App::Initialize();

	while (!g_app->IsQuitting())
	{
		g_app->RunFrame();
	}

	App::Shutdown();
	return 0;
}
