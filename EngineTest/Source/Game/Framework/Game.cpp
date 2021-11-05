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
#include "Engine/Render/ForwardRenderer.h"
#include "Engine/Render/Material/Material.h"
#include "Engine/Render/Renderable.h"
#include "Engine/Render/RenderContext.h"
#include "Engine/Render/RenderScene.h"
#include "Engine/Resource/ResourceSystem.h"
#include "Engine/Time/Clock.h"
#include "Engine/Render/Skybox.h"

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
	SAFE_DELETE(m_renderer);
	SAFE_DELETE(m_renderScene);
	SAFE_DELETE(m_uiCamera);
	SAFE_DELETE(m_gameCamera);
	SAFE_DELETE(m_gameClock);
}


//-------------------------------------------------------------------------------------------------
void Game::ProcessInput()
{
	m_player->ProcessInput(m_gameClock->GetDeltaSeconds());

	if (g_inputSystem->WasKeyJustPressed('I'))
	{
		m_drawColliders = !m_drawColliders;
	}

	Mouse& mouse = g_inputSystem->GetMouse();
	if (mouse.GetMouseWheelDelta() != 0.f)
	{
		const int numTypes = 3;
		if (mouse.GetMouseWheelDelta() > 0.f)
		{
			m_spawnType--;
			if (m_spawnType < 0)
			{
				m_spawnType = numTypes - 1;
			}
		}
		else
		{
			m_spawnType++;
			if (m_spawnType >= numTypes)
			{
				m_spawnType = 0;
			}
		}
	}

	if (mouse.WasButtonJustPressed(MOUSEBUTTON_LEFT))
	{
		Vector3 spawnPosition = m_gameCamera->GetPosition() + 2.f * m_gameCamera->GetForwardVector();
		Vector3 velocity = 10.f * m_gameCamera->GetForwardVector();
		float mass = 1.f;

		switch (m_spawnType)
		{
		case 0:
			SpawnBox(Vector3(0.5f), 1.0f / mass, spawnPosition, Vector3::ZERO, velocity);
			break;
		case 1:
			SpawnSphere(0.5f, 1.f / mass, spawnPosition, Vector3::ZERO, velocity);
			break;
		case 2:
			SpawnCapsule(0.5f, 0.25f, 1.f / mass, spawnPosition, Vector3::ZERO, velocity);
		default:
			break;
		}
	}
}


//-------------------------------------------------------------------------------------------------
void Game::Update()
{	
	const float deltaSeconds = m_gameClock->GetDeltaSeconds();
	
	switch (m_spawnType)
	{
	case 0:
		ConsolePrintf(Rgba::CYAN, 0.f, "Spawn Type: Box");
		break;
	case 1:
		ConsolePrintf(Rgba::CYAN, 0.f, "Spawn Type: Sphere");
		break;
	case 2: 
		ConsolePrintf(Rgba::CYAN, 0.f, "Spawn Type: Capsule");
		break;
	default:
		break;
	}

	PreUpdate(deltaSeconds);
	PhysicsUpdate(deltaSeconds);
	PostUpdate(deltaSeconds);
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	m_renderer->Render(m_renderScene);

	
	g_renderContext->BeginCamera(m_gameCamera);

	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/entity.material");
	Mesh* mesh = g_resourceSystem->CreateOrGetMesh("cylinder");
	Renderable rend;
	rend.AddDraw(mesh, material);
	rend.SetModelMatrix(Matrix4::MakeModelMatrix(Vector3(0.f, 3.f, 0.f), Quaternion::IDENTITY, Vector3::ONES));
	g_renderContext->DrawRenderable(rend);

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

	m_collisionScene = new CollisionScene<BoundingVolumeSphere>();
	m_physicsScene = new PhysicsScene(m_collisionScene);
}


//-------------------------------------------------------------------------------------------------
void Game::SetupRendering()
{
	// Cameras
	m_gameCamera = new Camera();
	m_gameCamera->SetProjectionPerspective(90.f, g_window->GetClientAspect(), 0.1f, 100.f);
	m_gameCamera->LookAt(Vector3(0.f, 0.f, -10.f), Vector3(0.f, 0.f, 0.f));
	m_gameCamera->SetColorTargetView(g_renderContext->GetDefaultColorTargetView());
	m_gameCamera->SetDepthStencilView(g_renderContext->GetDefaultDepthStencilView());
	g_debugRenderSystem->SetCamera(m_gameCamera);

	m_uiCamera = new Camera();
	m_uiCamera->SetProjectionOrthographic((float)g_window->GetClientPixelHeight(), g_window->GetClientAspect());

	m_renderScene = new RenderScene("Game");
	m_renderScene->AddCamera(m_gameCamera);
	Skybox* skybox = new Skybox(g_resourceSystem->CreateOrGetMaterial("Data/Material/skybox.material"));
	m_renderScene->SetSkybox(skybox);
	m_renderScene->SetAmbience(Rgba(255, 255, 255, 20));

	m_renderer = new ForwardRenderer();
}


//-------------------------------------------------------------------------------------------------
void Game::SpawnEntities()
{
	// Create the player
	m_player = new Player(m_gameCamera);
	m_collisionScene->AddEntity(m_player);
	m_physicsScene->AddRigidbody(m_player->rigidBody);
	m_entities.push_back(m_player);

	SpawnBox(Vector3(1.f), (1.f / 1.f),	 Vector3(-10.f, 1.f, 5.f));
	SpawnBox(Vector3(1.f), (1.f / 1.f),	 Vector3(-10.f, 1.f, 3.f));
	SpawnBox(Vector3(1.f), (1.f / 1.f),	 Vector3(-10.f, 1.f, 7.f));
	SpawnBox(Vector3(4.f), (1.f / 64.f), Vector3(10.f, 4.f, 5.f), Vector3(90.f, 0.f, 0.f));

	// Set up ground
	SpawnGround();
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

	// Ensure ground is below player
	m_ground->transform.position = m_player->transform.position;;
	m_ground->transform.position.y = 0.f;
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
	entity->collider = new CapsuleCollider(entity, Capsule3D(Vector3(0.f, -0.5f * cylinderHeight, 0.f), Vector3(0.f, 0.5f * cylinderHeight, 0.f), radius));

	m_collisionScene->AddEntity(entity);
	m_physicsScene->AddRigidbody(body);
	m_entities.push_back(entity);

	DebugRenderOptions options;
	options.m_parentTransform = &entity->transform;
	DebugDrawCapsule(Capsule3D(Vector3(0.f, -0.5f * cylinderHeight, 0.f), Vector3(0.f, 0.5f * cylinderHeight, 0.f), radius), options);
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

	DebugRenderOptions options;
	options.m_parentTransform = &entity->transform;
	DebugDrawBox(Vector3::ZERO, extents, Quaternion::IDENTITY, options);
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

	DebugRenderOptions options;
	options.m_parentTransform = &entity->transform;
	DebugDrawSphere(Vector3::ZERO, radius, options);
}


//-------------------------------------------------------------------------------------------------
// Spawns the entity used to represent the ground
void Game::SpawnGround()
{
	m_ground = new Entity();
	m_ground->collider = new HalfSpaceCollider(m_ground, Plane3(Vector3::Y_AXIS, Vector3::ZERO));;
	m_collisionScene->AddEntity(m_ground);
	m_entities.push_back(m_ground);

	// Place ground under the player
	m_ground->transform.position = m_player->transform.position;
	m_ground->transform.position.y = 0.f;

	Renderable rend(m_ground->GetId());
	Mesh* quad = g_resourceSystem->CreateOrGetMesh("horizontal_quad");
	Matrix4 rendModel = Matrix4::MakeModelMatrix(m_ground->transform.position, Vector3::ZERO, Vector3(100.f));
	rend.SetModelMatrix(rendModel);

	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/ground.material");
	rend.AddDraw(quad, material);

	m_renderScene->AddRenderable(m_ground->GetId(), rend);
}


//-------------------------------------------------------------------------------------------------
// Spawns a test light
void Game::SpawnLight()
{
	//Light* light = Light::CreateConeLight(m_gameCamera->GetPosition(), m_gameCamera->GetForwardVector(), 90.f, 70.f);
	//light->SetIsShadowCasting(true);
	//m_renderScene->AddLight(light);

	//Light* pointLight = Light::CreatePointLight(Vector3(0.f, 2.f, 5.f), Rgba::YELLOW);
	//pointLight->SetIsShadowCasting(true);
	//m_renderScene->AddLight(pointLight);

	//Light* dirLight = Light::CreateDirectionalLight(Vector3(0.f, 0.f, 0.f), Vector3(0.f, -1.f, 1.f), Rgba(255, 140, 0, 150));
	//dirLight->SetIsShadowCasting(true);
	//m_renderScene->AddLight(dirLight);
}
