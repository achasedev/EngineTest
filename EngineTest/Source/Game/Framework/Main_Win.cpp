#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Game/Framework/App.h"
#include "Engine/Framework/EngineCommon.h"

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int)
{
	UNUSED(commandLineString);
	UNUSED(applicationInstanceHandle);

	App::Initialize();
	App* theApp = App::GetInstance();

	while (!theApp->IsQuitting())
	{
		theApp->RunFrame();
	}

	App::Shutdown();
	return 0;
}
