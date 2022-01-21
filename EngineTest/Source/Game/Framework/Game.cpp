///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 18th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Block/BlockDefinition.h"
#include "Game/Chunk/Chunk.h"
#include "Game/Chunk/ChunkMeshBuilder.h"
#include "Game/Framework/Game.h"
#include "Game/Framework/World.h"
#include "Engine/Core/Window.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Render/Camera.h"
#include "Engine/Render/Debug/DebugRenderSystem.h"
#include "Engine/Render/ForwardRenderer.h"
#include "Engine/Render/Renderable.h"
#include "Engine/Render/RenderContext.h"
#include "Engine/Render/RenderScene.h"
#include "Engine/Resource/ResourceSystem.h"
#include "Engine/Render/Skybox.h"
#include "Engine/Time/Clock.h"

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------
Game* g_game = nullptr;

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS IMPLEMENTATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
Game::Game()
{
}


//-------------------------------------------------------------------------------------------------
void Game::StartUp()
{
	SetupFramework();
	SetupRendering();

	BlockDefinition::InitializeBuiltInDefs();

	m_world = new World();
	m_world->Initialize();
}


//-------------------------------------------------------------------------------------------------
Game::~Game()
{
	g_debugRenderSystem->SetCamera(nullptr);

	SAFE_DELETE(m_world);
	SAFE_DELETE(m_renderer);
	SAFE_DELETE(m_gameCamera);
	SAFE_DELETE(m_gameClock);
}


//-------------------------------------------------------------------------------------------------
void Game::ProcessInput()
{
	// Translation input
	Vector3 moveDir = Vector3::ZERO;
	if (g_inputSystem->IsKeyPressed('W'))								{ moveDir.z += 1.f; }		// Forward
	if (g_inputSystem->IsKeyPressed('S'))								{ moveDir.z -= 1.f; }		// Left
	if (g_inputSystem->IsKeyPressed('A'))								{ moveDir.x -= 1.f; }		// Back
	if (g_inputSystem->IsKeyPressed('D'))								{ moveDir.x += 1.f; }		// Right
	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SPACEBAR))	{ moveDir.y += 1.f; }		// Up
	if (g_inputSystem->IsKeyPressed('X'))								{ moveDir.y -= 1.f; }		// Down
	moveDir.SafeNormalize(moveDir);

	float scale = 10.f;
	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SHIFT))
	{
		scale = 20.f;
	}
	else if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_CONTROL))
	{
		scale = 5.f;
	}

	// Rotation input
	Mouse& mouse = InputSystem::GetMouse();
	IntVector2 mouseDelta = mouse.GetMouseDelta();
	Vector2 rot = Vector2((float)mouseDelta.y, (float)mouseDelta.x); // Flip X and Y

	float deltaSeconds = m_gameClock->GetDeltaSeconds();
	const float degreesPerSecond = 30.f;
	Vector3 deltaDegrees = Vector3(rot.x, rot.y, 0.f) * degreesPerSecond * deltaSeconds;

	m_gameCamera->Translate(moveDir * scale * deltaSeconds);

	Vector3 cameraDegrees = m_gameCamera->GetRotationAsEulerAnglesDegrees() + Vector3(deltaDegrees.x, deltaDegrees.y, 0.f);
	m_gameCamera->SetRotationEulerAnglesDegrees(cameraDegrees);

	m_world->ProcessInput();
}


//-------------------------------------------------------------------------------------------------
void Game::Update()
{	
	float deltaSeconds = m_gameClock->GetDeltaSeconds();
	m_world->Update(deltaSeconds);
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	RenderScene* scene = m_world->GetRenderScene();
	m_renderer->Render(scene);
}


//-------------------------------------------------------------------------------------------------
void Game::SetupFramework()
{
	Mouse& mouse = InputSystem::GetMouse();
	mouse.ShowMouseCursor(false);
	mouse.LockCursorToClient(true);
	mouse.SetCursorMode(CURSORMODE_RELATIVE);

	m_gameClock = new Clock(nullptr);
}


//-------------------------------------------------------------------------------------------------
void Game::SetupRendering()
{
	// Cameras
	m_gameCamera = new Camera();
	m_gameCamera->SetProjectionPerspective(90.f, g_window->GetClientAspect(), 0.1f, 100.f);
	m_gameCamera->LookAt(Vector3(0.f, 1.f, -1.f), Vector3(0.f, 0.f, 0.f));
	m_gameCamera->SetColorTargetView(g_renderContext->GetDefaultColorTargetView());
	m_gameCamera->SetDepthStencilView(g_renderContext->GetDefaultDepthStencilView());
	g_debugRenderSystem->SetCamera(m_gameCamera);
	
	m_renderer = new ForwardRenderer();
}
