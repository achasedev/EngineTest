///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 18th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Framework/Game.h"
#include "Game/Framework/GameJobs.h"
#include "Engine/Core/DevConsole.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/Entity.h"
#include "Engine/Core/Window.h"
#include "Engine/IO/Image.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Job/JobSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/OBB2.h"
#include "Engine/Math/OBB3.h"
#include "Engine/Math/Polygon3d.h"
#include "Engine/Render/Camera.h"
#include "Engine/Render/Renderable.h"
#include "Engine/Render/RenderContext.h"
#include "Engine/Render/Font/Font.h"
#include "Engine/Render/Font/FontAtlas.h"
#include "Engine/Render/Material.h"
#include "Engine/Render/Mesh/Mesh.h"
#include "Engine/Render/Mesh/MeshBuilder.h"
#include "Engine/Render/Shader.h"
#include "Engine/Render/Texture/Texture2D.h"
#include "Engine/Time/Clock.h"
#include "Engine/Time/FrameTimer.h"
#include "Engine/Time/Time.h"
#include "Engine/Utility/NamedProperties.h"
#include "Engine/Utility/SmartPointer.h"
#include "Engine/Utility/StringID.h"
#include "Engine/UI/Canvas.h"
#include "Engine/UI/UIPanel.h"
#include "Engine/UI/UIText.h"
#include "Engine/Voxel/QEFLoader.h"
#include "Engine/Render/Debug/DebugRenderSystem.h"
#include "Engine/Resource/ResourceSystem.h"

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------

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
}


//-------------------------------------------------------------------------------------------------
Game::~Game()
{
	g_debugRenderSystem->SetCamera(nullptr);

	SAFE_DELETE(m_uiCamera);
	SAFE_DELETE(m_gameCamera);
	SAFE_DELETE(m_gameClock);
}


struct AxisResult
{
	float m_distance;
	Plane3 m_plane;
	const HalfEdge* m_edgeA;
	const HalfEdge* m_edgeB;
	Vector3 m_supportPoint;
	float m_distanceCenterToPlane;
};


//-------------------------------------------------------------------------------------------------
void Game::ProcessInput()
{
	static Vector3 vel = Vector3::ZERO;

	// Translating the camera
	Vector3 moveDir = Vector3::ZERO;
	if (g_inputSystem->IsKeyPressed('W')) { moveDir.z += 1.f; }		// Forward
	if (g_inputSystem->IsKeyPressed('S')) { moveDir.z -= 1.f; }		// Left
	if (g_inputSystem->IsKeyPressed('A')) { moveDir.x -= 1.f; }		// Back
	if (g_inputSystem->IsKeyPressed('D')) { moveDir.x += 1.f; }		// Right
	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SPACEBAR)) { moveDir.y += 1.f; }		// Up
	if (g_inputSystem->IsKeyPressed('X')) { moveDir.y -= 1.f; }		// Down
	moveDir.SafeNormalize(moveDir);

	const float deltaSeconds = m_gameClock->GetDeltaSeconds();
	const float speed = (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SHIFT) ? 5.f : 25.f);

	m_gameCamera->Translate(moveDir * speed * deltaSeconds);

	// Rotating the camera
	Mouse& mouse = InputSystem::GetMouse();
	IntVector2 mouseDelta = mouse.GetMouseDelta();
	Vector2 rot = Vector2((float)mouseDelta.y, (float)mouseDelta.x); // Flip X and Y

	const float rotSpeed = 120.f;
	Vector3 deltaRot = Vector3(rot.x, rot.y, 0.f) * rotSpeed * deltaSeconds;
	m_gameCamera->SetRotation(m_gameCamera->GetRotation() + deltaRot);
}


//-------------------------------------------------------------------------------------------------
void Game::Update()
{
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

	// Draw here

	g_renderContext->EndCamera();
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
	m_gameCamera->SetProjectionPerspective(90.f, 0.1f, 100.f);
	m_gameCamera->LookAt(Vector3(0.f, 0.f, -10.f), Vector3(0.f, 0.f, 0.f));
	m_gameCamera->SetDepthTarget(g_renderContext->GetDefaultDepthStencilTarget(), false);
	g_debugRenderSystem->SetCamera(m_gameCamera);

	m_uiCamera = new Camera();
	m_uiCamera->SetProjectionOrthographic((float)g_window->GetClientPixelHeight(), g_window->GetClientAspect());

	DebugDrawCube(Vector3(2.f, 2.f, 2.f), Vector3(0.5f));
	DebugDrawCube(Vector3(2.f, 2.f, -2.f), Vector3(0.5f));
	DebugDrawCube(Vector3(2.f, -2.f, 2.f), Vector3(0.5f));
	DebugDrawCube(Vector3(2.f, -2.f, -2.f), Vector3(0.5f));
	DebugDrawCube(Vector3(-2.f, 2.f, 2.f), Vector3(0.5f));
	DebugDrawCube(Vector3(-2.f, 2.f, -2.f), Vector3(0.5f));
	DebugDrawCube(Vector3(-2.f, -2.f, 2.f), Vector3(0.5f));
	DebugDrawCube(Vector3(-2.f, -2.f, -2.f), Vector3(0.5f));
}
