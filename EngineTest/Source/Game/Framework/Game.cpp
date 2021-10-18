///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 18th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Entity/BlockObject.h"
#include "Game/Entity/Player.h"
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
#include "Engine/Render/Renderable.h"
#include "Engine/Render/RenderContext.h"
#include "Engine/Resource/ResourceSystem.h"
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
	SpawnEntities();
}


//-------------------------------------------------------------------------------------------------
Game::~Game()
{
	g_debugRenderSystem->SetCamera(nullptr);

	for (Entity* entity : m_entities)
	{
		if (entity->collider != nullptr)
		{	
			m_collisionScene->RemoveEntity(entity);
			SAFE_DELETE(entity->collider);
		}
	}

	SafeDeleteVector(m_entities);

	SAFE_DELETE(m_collisionScene);
	SAFE_DELETE(m_physicsScene);
	SAFE_DELETE(m_uiCamera);
	SAFE_DELETE(m_gameCamera);
	SAFE_DELETE(m_gameClock);
}


//-------------------------------------------------------------------------------------------------
void Game::ProcessInput()
{
	m_player->ProcessInput(m_gameClock->GetDeltaSeconds());
	//float mass = 1.f;
	//float iMass = (1.f / mass);

	//Vector3 spawnPosition = m_gameCamera->GetPosition() + m_gameCamera->GetForwardVector() * 1.f;
	//Vector3 spawnRotation = m_gameCamera->GetRotationAsEulerAnglesDegrees();
	//Vector3 spawnVelocity = m_gameCamera->GetForwardVector() * 20.f;
	////Vector3 spawnVelocity = Vector3::ZERO;
	//Vector3 spawnAngularVelocityDegrees = Vector3::ZERO;

	//if (mouse.WasButtonJustPressed(MOUSEBUTTON_LEFT))
	//{
	//	SpawnBox(Vector3(0.5f), iMass, spawnPosition, spawnRotation, spawnVelocity, spawnAngularVelocityDegrees, true);
	//}

	//if (mouse.WasButtonJustPressed(MOUSEBUTTON_RIGHT))
	//{
	//	SpawnSphere(0.5f, iMass, spawnPosition, spawnRotation, spawnVelocity, spawnAngularVelocityDegrees, true);
	//}

	//if (mouse.WasButtonJustPressed(MOUSEBUTTON_MIDDLE))
	//{
	//	SpawnCapsule(1.f, 0.5f, iMass, spawnPosition, spawnRotation, spawnVelocity, spawnAngularVelocityDegrees, true);
	//}
}


//-------------------------------------------------------------------------------------------------
void Game::Update()
{	
	const float deltaSeconds = m_gameClock->GetDeltaSeconds();
	
	PreUpdate(deltaSeconds);
	PhysicsUpdate(deltaSeconds);
	PostUpdate(deltaSeconds);
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

	for (Entity* entity : m_entities)
	{
		entity->Render();
	}

	Material* skyboxMaterial = g_resourceSystem->CreateOrGetMaterial("Data/Material/skybox.material");
	Mesh* skyboxMesh = g_resourceSystem->CreateOrGetMesh("unit_cube");

	Vector3 pos = m_player->transform.position;
	pos.y = 0.f;
	Mesh* quad = g_resourceSystem->CreateOrGetMesh("horizontal_quad");
	Matrix4 model = Matrix4::MakeModelMatrix(pos, Vector3::ZERO, Vector3(100.f));
	Renderable rend;
	rend.SetRenderableMatrix(model);

	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/ground.material");
	rend.AddDraw(quad, material);
	g_renderContext->DrawRenderable(rend);

	g_renderContext->DrawMeshWithMaterial(*skyboxMesh, skyboxMaterial);
	
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
void Game::SpawnEntities()
{
	m_collisionScene = new CollisionScene<BoundingVolumeSphere>();
	m_physicsScene = new PhysicsScene(m_collisionScene);

	m_ground = new Entity();
	m_ground->collider = new HalfSpaceCollider(m_ground, Plane3(Vector3::Y_AXIS, Vector3::ZERO));;

	m_collisionScene->AddEntity(m_ground);
	m_entities.push_back(m_ground);

	SpawnBox(Vector3(1.f),	(1.f / 1.f),	Vector3(-10.f, 1.f, 0.f));
	//SpawnBox(Vector3(2.f), (1.f / 8.f),		Vector3(0.f, 2.f, 0.f));
	SpawnBox(Vector3(4.f), (1.f / 64.f),	Vector3(10.f, 4.f, 0.f));

	m_player = new Player(m_gameCamera);
	m_collisionScene->AddEntity(m_player);
	m_physicsScene->AddRigidbody(m_player->rigidBody);
	m_entities.push_back(m_player);

	BlockObject* obj = new BlockObject();
	obj->transform.position = Vector3(0.f, 1.5f, 0.f);
	m_entities.push_back(obj);
}


//-------------------------------------------------------------------------------------------------
void Game::PreUpdate(float deltaSeconds)
{
	for (Entity* entity : m_entities)
	{
		entity->PreUpdate(deltaSeconds);
	}
}


//-------------------------------------------------------------------------------------------------
void Game::PhysicsUpdate(float deltaSeconds)
{
	m_physicsScene->BeginFrame();
	m_physicsScene->DoPhysicsStep(deltaSeconds);
}


//-------------------------------------------------------------------------------------------------
void Game::PostUpdate(float deltaSeconds)
{
	for (Entity* entity : m_entities)
	{
		entity->PostUpdate(deltaSeconds);
	}
}


//-------------------------------------------------------------------------------------------------
void Game::SpawnCapsule(float cylinderHeight, float radius, float inverseMass, const Vector3& position, const Vector3& rotationDegrees /*= Vector3::ZERO*/, const Vector3& velocity /*= Vector3::ZERO*/, const Vector3& angularVelocityDegrees /*= Vector3::ZERO*/, bool hasGravity /*= true*/)
{
	Entity* entity = new Entity();
	entity->transform.position = position;
	entity->transform.rotation = Quaternion::CreateFromEulerAnglesDegrees(rotationDegrees);

	RigidBody* body = new RigidBody(&entity->transform);
	body->SetInverseMass(inverseMass);
	body->SetInertiaTensor_Capsule(cylinderHeight, radius);
	body->SetVelocityWs(velocity);
	body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
	body->SetAffectedByGravity(hasGravity);

	entity->rigidBody = body;
	entity->collider = new CapsuleCollider(entity, Capsule3D(Vector3(0.f, -cylinderHeight, 0.f), Vector3(0.f, cylinderHeight, 0.f), radius));

	m_collisionScene->AddEntity(entity);
	m_physicsScene->AddRigidbody(body);
	m_entities.push_back(entity);
}


//-------------------------------------------------------------------------------------------------
void Game::SpawnBox(const Vector3& extents, float inverseMass, const Vector3& position, const Vector3& rotationDegrees /*= Vector3::ZERO*/, const Vector3& velocity /*= Vector3::ZERO*/, const Vector3& angularVelocityDegrees /*= Vector3::ZERO*/, bool hasGravity /*= true*/)
{
	Entity* entity = new Entity();
	entity->transform.position = position;
	entity->transform.rotation = Quaternion::CreateFromEulerAnglesDegrees(rotationDegrees);

	RigidBody* body = new RigidBody(&entity->transform);
	body->SetInverseMass(inverseMass);
	body->SetInertiaTensor_Box(extents);
	body->SetVelocityWs(velocity);
	body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
	body->SetAffectedByGravity(hasGravity);

	entity->rigidBody = body;
	entity->collider = new BoxCollider(entity, OBB3(Vector3::ZERO, extents, Quaternion::IDENTITY));

	m_collisionScene->AddEntity(entity);
	m_physicsScene->AddRigidbody(body);
	m_entities.push_back(entity);
}


//-------------------------------------------------------------------------------------------------
void Game::SpawnSphere(float radius, float inverseMass, const Vector3& position, const Vector3& rotationDegrees /*= Vector3::ZERO*/, const Vector3& velocity /*= Vector3::ZERO*/, const Vector3& angularVelocityDegrees /*= Vector3::ZERO*/, bool hasGravity /*= true*/)
{
	Entity* entity = new Entity();
	entity->transform.position = position;
	entity->transform.rotation = Quaternion::CreateFromEulerAnglesDegrees(rotationDegrees);

	RigidBody* body = new RigidBody(&entity->transform);
	body->SetInverseMass(inverseMass);
	body->SetInertiaTensor_Sphere(radius);
	body->SetVelocityWs(velocity);
	body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
	body->SetAffectedByGravity(hasGravity);

	entity->rigidBody = body;
	entity->collider = new SphereCollider(entity, Sphere3D(Vector3::ZERO, radius));

	m_collisionScene->AddEntity(entity);
	m_physicsScene->AddRigidbody(body);
	m_entities.push_back(entity);
}
