///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 18th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Block/BlockDefinition.h"
#include "Game/Block/Chunk.h"
#include "Game/Framework/Game.h"
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
	SetupFramework();
	SetupRendering();
	BlockDefinition::InitializeBuiltInDefs();
	m_chunk = new Chunk(IntVector3(0,0,0));
	m_chunk->GenerateWithNoise(16, 10, 12);
	m_chunk->BuildBetterMesh();
	
	Renderable rend;
	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/chunk.material");
	rend.AddDraw(m_chunk->GetMesh(), material);

	m_renderScene->AddRenderable(400, rend);
	DebugRenderOptions options;
	options.m_startColor = Rgba::RED;
	options.m_endColor = Rgba::RED;
	options.m_fillMode = FILL_MODE_WIREFRAME;
	options.m_debugRenderMode = DEBUG_RENDER_MODE_XRAY;
	
	DebugDrawBox(OBB3(m_chunk->GetBoundsWs()), options);
}


//-------------------------------------------------------------------------------------------------
Game::~Game()
{
	g_debugRenderSystem->SetCamera(nullptr);

	SAFE_DELETE(m_renderer);
	SAFE_DELETE(m_renderScene);
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
}


//-------------------------------------------------------------------------------------------------
void Game::Update()
{	
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	m_renderer->Render(m_renderScene);
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

	m_renderScene = new RenderScene("World");
	m_renderScene->AddCamera(m_gameCamera);

	Skybox* skybox = new Skybox(g_resourceSystem->CreateOrGetMaterial("Data/Material/skybox.material"));
	m_renderScene->SetSkybox(skybox);
	m_renderScene->SetAmbience(Rgba(255, 255, 255, 200));
	
	Light* dirLight = Light::CreateDirectionalLight(Vector3::ZERO, Vector3(0.f, -1.f, 1.f).GetNormalized(), Rgba(255, 255, 220, 255));
	dirLight->SetIsShadowCasting(true);
	m_renderScene->AddLight(dirLight);

	m_renderer = new ForwardRenderer();

	DebugDrawBox(Vector3::ZERO, Vector3(0.5f), Quaternion::IDENTITY);
}
