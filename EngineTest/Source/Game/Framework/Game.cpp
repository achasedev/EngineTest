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
#include "Engine/Physics/RigidBody/RigidBody.h"
#include "Engine/Physics/RigidBody/PhysicsScene.h"
#include "Engine/Physics/RigidBody/RigidBodyAnchoredSpring.h"
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
#include "Engine/Math/Matrix3.h"
#include "Engine/Math/OBB3.h"

//-------------------------------------------------------------------------------------------------
Game::Game()
{
	SetupFramework();
	SetupRendering();
	SetupParticles();
	SetupRigidBodies();

	m_parent.position = Vector3(0.f, 10.f, -10.f);
	//m_parent.rotation = Quaternion::CreateFromAxisAndDegreeAngle(Vector3::ONES, 90.f);
	m_child.position = Vector3(2.f, 0.f, 0.f);
	//m_child.rotation = Quaternion::CreateFromEulerAnglesDegrees(45.f, 0.f, 0.f);
	m_child.SetParentTransform(&m_parent);

	Transform identityTransform;
	DebugDrawTransform(identityTransform, 99999.f, &m_parent);
	DebugDrawTransform(identityTransform, 99999.f, &m_child);

	Vector3 startingAngles = Vector3(45.f, 60.f, 15.f);

	Quaternion quatFromAngles = Quaternion::CreateFromEulerAnglesDegrees(startingAngles);

	Matrix3 mat3FromAngles = Matrix3::MakeRotationFromEulerAnglesDegrees(startingAngles);
	Matrix3 mat3FromQuat = Matrix3(quatFromAngles);

	Matrix4 mat4FromAngles = Matrix4::MakeRotationFromEulerAnglesDegrees(startingAngles);
	Matrix4 mat4FromQuat = Matrix4::MakeRotation(quatFromAngles);
	Matrix4 mat4FromMat3 = Matrix4(mat3FromAngles);

	Vector3 anglesFromQuat = quatFromAngles.GetAsEulerAnglesDegrees();
	Vector3 anglesFromMat3Angles = Matrix3::ExtractRotationAsEulerAnglesDegrees(mat3FromAngles);
	Vector3 anglesFromMat3Quat = Matrix3::ExtractRotationAsEulerAnglesDegrees(mat3FromQuat);

	Vector3 anglesFromMat4Angles = Matrix4::ExtractRotationAsEulerAnglesDegrees(mat4FromAngles);
	Vector3 anglesFromMat4Mat3 = Matrix4::ExtractRotationAsEulerAnglesDegrees(mat4FromMat3);
	Vector3 anglesFromMat4Quat = Matrix4::ExtractRotationAsEulerAnglesDegrees(mat4FromQuat);

	Matrix4 inv1 = mat4FromAngles;
	inv1.FastInverse();
	Matrix4 test1 = mat4FromAngles * inv1;

	Matrix4 inv2 = mat4FromQuat;
	inv2.FastInverse();
	Matrix4 test2 = mat4FromQuat * inv2;

	Matrix4 inv3 = mat4FromMat3;
	inv3.FastInverse();
	Matrix4 test3 = mat4FromMat3 * inv3;


	OBB3 box = OBB3(Vector3(0.f, 10.f, -20.f), Vector3::ONES, Quaternion::CreateFromEulerAnglesDegrees(0.f, 90.f, 0.f));

	Vector3 boxVerts[8];
	box.GetPoints(boxVerts);

	for (int i = 0; i < 8; ++i)
	{
		DebugDrawPoint3D(boxVerts[i], Rgba::MAGENTA);
	}

	Vector3 point = Vector3(0.f, 10.f, -7.f);
	Vector3 posRelative = box.TransformPositionIntoSpace(point);

}


//-------------------------------------------------------------------------------------------------
Game::~Game()
{
	g_debugRenderSystem->SetCamera(nullptr);

	for (int i = 0; i < 10; ++i)
	{
		m_collisionScene->RemoveEntity(m_entities[i]);
	}

	SAFE_DELETE(m_collisionScene);

	for (int i = 0; i < 10; ++i)
	{
		SAFE_DELETE(m_entities[i]);
	}

	SAFE_DELETE(m_rigidBodyScene);
	SAFE_DELETE(m_particleWorld);
	SAFE_DELETE(m_uiCamera);
	SAFE_DELETE(m_gameCamera);
	SAFE_DELETE(m_gameClock);
}


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

	const float degreesPerSecond = 120.f;
	Vector3 deltaDegrees = Vector3(rot.x, rot.y, 0.f) * degreesPerSecond * deltaSeconds;
	//m_gameCamera->RotateEulerAnglesDegrees(deltaDegrees);
	m_gameCamera->SetRotationEulerAnglesDegrees(m_gameCamera->GetRotationAsEulerAnglesDegrees() + deltaDegrees);
}


//-------------------------------------------------------------------------------------------------
void Game::Update()
{
	const float deltaSeconds = m_gameClock->GetDeltaSeconds();

	// Generate forces
	m_particleWorld->DoPhysicsStep(deltaSeconds);
	m_particleWorld->DebugDrawParticles();

	m_rigidBodyScene->BeginFrame();
	m_rigidBodyScene->DoPhysicsStep(deltaSeconds);

	const float radiansPerSecond = PI;
	const float deltaRadians = radiansPerSecond * deltaSeconds;

	m_parent.Rotate(Quaternion::CreateFromAxisAndRadianAngle(Vector3::Y_AXIS, deltaRadians), RELATIVE_TO_WORLD);
	//m_child.RotateRadians(Vector3(0.f, deltaRadians, 0.f), RELATIVE_TO_WORLD);
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

	// Draw here
	for (int i = 0; i < 10; ++i)
	{
		m_entities[i]->Render();
	}

	g_renderContext->EndCamera();

	m_collisionScene->DebugRenderBoundingHierarchy();
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


//-------------------------------------------------------------------------------------------------
void Game::SetupRigidBodies()
{
	m_collisionScene = new CollisionScene<BoundingVolumeSphere>();
	m_rigidBodyScene = new PhysicsScene(m_collisionScene);

	for (int i = 0; i < 10; ++i)
	{
		Vector3 extents(0.5f);
		Vector3 pos = Vector3(5.f * extents.x * (float) i, -2.f * (float) i, 0.f);
		m_entities[i] = new Entity();

		RigidBody* body = new RigidBody(&m_entities[i]->transform);
		body->SetAcceleration(Vector3(0.f, -10.f, 0.f));
		body->transform->position = pos;
		const float mass = 10.0f;

		body->SetInverseMass((1.f / mass));
		Matrix3 inertiaTensor;
		inertiaTensor.Ix = (1.f / 12.f) * mass * (extents.y * extents.y + extents.z * extents.z);
		inertiaTensor.Jy = (1.f / 12.f) * mass * (extents.x * extents.x + extents.z * extents.z);
		inertiaTensor.Kz = (1.f / 12.f) * mass * (extents.x * extents.x + extents.y * extents.y);
		body->SetLocalInverseInertiaTensor(inertiaTensor.GetInverse());
		body->SetAngularDamping(0.4f);
		body->SetLinearDamping(0.75f);

		m_rigidBodyScene->AddRigidbody(body);
		RigidBodyAnchoredSpring* spring = new RigidBodyAnchoredSpring(extents, Vector3(pos.x, 0.f, pos.z), 15.f, 3.f * (float)i);

		m_rigidBodyScene->AddForceGenerator(spring, body);

		m_entities[i]->rigidBody = body;
		m_entities[i]->renderShapeLs = AABB3(Vector3::ZERO, extents.x, extents.y, extents.z);

		m_collisionScene->AddEntity(m_entities[i]);
	}
}
