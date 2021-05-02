///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 18th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Framework/Game.h"
#include "Engine/Core/DevConsole.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/Window.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Physics/Particle/Particle.h"
#include "Engine/Physics/Particle/ParticleAnchoredBungee.h"
#include "Engine/Physics/Particle/ParticleAnchoredSpring.h"
#include "Engine/Physics/Particle/ParticleBungee.h"
#include "Engine/Physics/Particle/ParticleBuoyancy.h"
#include "Engine/Physics/Particle/ParticleSpring.h"
#include "Engine/Physics/Particle/ParticleWorld.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Render/Camera.h"
#include "Engine/Render/Debug/DebugRenderSystem.h"
#include "Engine/Render/RenderContext.h"
#include "Engine/Resource/ResourceSystem.h"
#include "Engine/Time/Clock.h"
#include "Engine/Physics/Particle/ParticleRod.h"
#include "Engine/Physics/Particle/ParticleCable.h"

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
	SetupParticles();

	m_parent.position = Vector3(0.f, 10.f, -10.f);
	//m_parent.rotation = Quaternion::CreateFromAxisAndDegreeAngle(Vector3::ONES, 90.f);
	m_child.position = Vector3(2.f, 0.f, 0.f);
	m_child.rotation = Quaternion::CreateFromEulerAngles(45.f, 0.f, 0.f);
	m_child.SetParentTransform(&m_parent);

	Transform identityTransform;
	DebugDrawTransform(identityTransform, 99999.f, &m_parent);
	DebugDrawTransform(identityTransform, 99999.f, &m_child);
}


//-------------------------------------------------------------------------------------------------
Game::~Game()
{
	g_debugRenderSystem->SetCamera(nullptr);

	SAFE_DELETE(m_particleWorld);
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
	const float deltaSeconds = m_gameClock->GetDeltaSeconds();

	// Generate forces
	m_particleWorld->DoPhysicsStep(deltaSeconds);
	m_particleWorld->DebugDrawParticles();

	const float radiansPerSecond = PI;
	const float deltaRadians = radiansPerSecond * deltaSeconds;

	m_parent.Rotate(Quaternion::CreateFromAxisAndRadianAngle(Vector3::ONES, deltaRadians), RELATIVE_TO_WORLD);
	//m_child.RotateRadians(Vector3(0.f, deltaRadians, 0.f), RELATIVE_TO_WORLD);
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
}


//-------------------------------------------------------------------------------------------------
void Game::SetupParticles()
{
	m_particleWorld = new ParticleWorld(8, 8);

	Particle* anchorParticle = new Particle(Vector3::ZERO, Vector3::ZERO, 0.f, 0.f, Vector3::ZERO);
	Particle* moveParticle = new Particle(Vector3(-1.f, 0.f, 0.f), Vector3(0.f, 5.0f, 0.f), 1.f, 0.99f, Vector3(0.f, -10.f, 0.f));
	Particle* moveParticle2 = new Particle(Vector3(-2.f, 0.f, 0.f), Vector3(0.f, 0.0f, 0.f), 1.f, 0.99f, Vector3(0.f, -10.f, 0.f));

	m_particleWorld->AddParticle(anchorParticle);
	m_particleWorld->AddParticle(moveParticle);
	m_particleWorld->AddParticle(moveParticle2);

	//ParticleRod* rod = new ParticleRod(anchorParticle, moveParticle, 1.0f);
	//ParticleRod* rod2 = new ParticleRod(moveParticle, moveParticle2, 2.0f);
	//m_particleWorld->AddContactGenerator(rod);
	//m_particleWorld->AddContactGenerator(rod2);

	ParticleCable* cable = new ParticleCable(moveParticle, anchorParticle, 3.0f, 0.75f);
	ParticleCable* cable2 = new ParticleCable(moveParticle, moveParticle2, 1.0f, 0.75f);
	m_particleWorld->AddContactGenerator(cable);
	m_particleWorld->AddContactGenerator(cable2);
}
