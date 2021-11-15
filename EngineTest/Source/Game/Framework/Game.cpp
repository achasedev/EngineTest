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
#include "Engine/Render/Mesh/MeshBuilder.h"
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

	Mouse& mouse = g_inputSystem->GetMouse();
	if (mouse.GetMouseWheelDelta() != 0.f)
	{
		const int numTypes = 5;
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
			SpawnBox(Vector3(1.f, 1.f, 1.5f), 1.0f / mass, spawnPosition, Vector3::ZERO, velocity, Vector3::ZERO);
			break;
		case 1:
			SpawnSphere(0.5f, 1.f / mass, spawnPosition, Vector3::ZERO, velocity);
			break;
		case 2:
			SpawnCapsule(2.f, 1.5f, 1.f / mass, spawnPosition, Vector3(45.f, 0.f, 0.f), velocity);
			break;
		case 3:
			SpawnCylinder(1.5f, 2.f, 1.f / mass, spawnPosition, Vector3(0.f, 0.f, 0.f), velocity);
			break;
		case 4:
			SpawnPolygon(1.f / mass, spawnPosition, Vector3::ZERO, velocity, Vector3(-122.f, 72.f, 90.f));
			break;
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
	case 3:
		ConsolePrintf(Rgba::CYAN, 0.f, "Spawn Type: Cylinder");
		break;
	case 4:
		ConsolePrintf(Rgba::CYAN, 0.f, "Spawn Type: Polygon");
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
	m_renderScene->SetAmbience(Rgba(255, 255, 255, 200));
	
	Light* dirLight = Light::CreateDirectionalLight(Vector3::ZERO, Vector3(0.f, -1.f, 1.f).GetNormalized(), Rgba(255, 255, 220, 255));
	dirLight->SetIsShadowCasting(true);
	m_renderScene->AddLight(dirLight);

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

	//SpawnBox(Vector3(1.f), (1.f / 1.f),	 Vector3(-10.f, 1.f, 5.f));
	//SpawnBox(Vector3(1.f), (1.f / 1.f),	 Vector3(-10.f, 1.f, 3.f));
	//SpawnBox(Vector3(1.f), (1.f / 1.f),	 Vector3(-10.f, 1.f, 7.f));
	SpawnBox(Vector3(10.f), (1.f / 2.f), Vector3(10.f, 4.f, 5.f), Vector3(180.f, 0.f, 0.f));
	SpawnCylinder(5.f, 5.f, 0.f, Vector3(0.f, 2.5f, 20.f), Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, false);

	// Set up ground
	SpawnGround();
	
	m_poly.AddVertex(Vector3(-1.f, -1.f, -1.f));
	m_poly.AddVertex(Vector3(-1.f, 0.f, -1.f));
	m_poly.AddVertex(Vector3(1.f, -1.f, -1.f));
	m_poly.AddVertex(Vector3(1.f, -1.f, 1.f));
	m_poly.AddVertex(Vector3(-1.f, 0.f, +1.f));
	m_poly.AddVertex(Vector3(-1.f, -1.f, +2.f));

	std::vector<int> face0{ 0, 1, 2 };
	std::vector<int> face1{ 3, 4, 5 };
	std::vector<int> face2{ 5, 4, 1, 0 };
	std::vector<int> face3{ 5, 0, 2, 3 };
	std::vector<int> face4{ 2, 1, 4, 3 };

	m_poly.AddFace(face0);
	m_poly.AddFace(face1);
	m_poly.AddFace(face2);
	m_poly.AddFace(face3);
	m_poly.AddFace(face4);

	MeshBuilder mb;
	mb.BeginBuilding(TOPOLOGY_TRIANGLE_LIST, true);
	mb.PushPolygon(m_poly);
	mb.GenerateFlatNormals();
	mb.FinishBuilding();

	m_polyMesh = mb.CreateMesh<VertexLit>();
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

	UpdateRenderables();
}


//-------------------------------------------------------------------------------------------------
void Game::SpawnCapsule(float cylinderHeight, float radius, float inverseMass, const Vector3& position, const Vector3& rotationDegrees /*= Vector3::ZERO*/, const Vector3& velocity /*= Vector3::ZERO*/, const Vector3& angularVelocityDegrees /*= Vector3::ZERO*/, bool hasGravity /*= true*/)
{
	Entity* entity = new Entity();
	entity->transform.position = position;
	entity->transform.rotation = Quaternion::CreateFromEulerAnglesDegrees(rotationDegrees);
	entity->transform.scale = Vector3(radius, cylinderHeight, radius);

	RigidBody* body = new RigidBody(&entity->transform);
	body->SetInverseMass(inverseMass);
	body->SetInertiaTensor_Capsule(cylinderHeight, radius);

	body->SetVelocityWs(velocity);
	body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
	body->SetAffectedByGravity(hasGravity);

	entity->rigidBody = body;
	entity->collider = new CapsuleCollider(entity, Capsule3D(Vector3(0.f, -0.5f, 0.f), Vector3(0.f, 0.5f, 0.f), 1.0f));

	m_collisionScene->AddEntity(entity);
	m_physicsScene->AddRigidbody(body);
	m_entities.push_back(entity);

	Mesh* bottom = g_resourceSystem->CreateOrGetMesh("capsule_bottom");
	Mesh* middle = g_resourceSystem->CreateOrGetMesh("capsule_middle");
	Mesh* top = g_resourceSystem->CreateOrGetMesh("capsule_top");

	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/dot3.material");

	Matrix4 bottomMat = Matrix4::MakeModelMatrix(Vector3(0.f, -0.5f * cylinderHeight, 0.f), Quaternion::IDENTITY, Vector3(radius));
	Matrix4 middleMat = Matrix4::MakeModelMatrix(Vector3::ZERO, Quaternion::IDENTITY, Vector3(radius, cylinderHeight, radius));
	Matrix4 topMat = Matrix4::MakeModelMatrix(Vector3(0.f, 0.5f * cylinderHeight, 0.f), Quaternion::IDENTITY, Vector3(radius));

	Renderable rend;
	rend.SetModelMatrix(entity->transform.GetModelMatrix());
	rend.AddDraw(bottom, material, bottomMat);
	rend.AddDraw(middle, material, middleMat);
	rend.AddDraw(top, material, topMat);
	m_renderScene->AddRenderable(entity->GetId(), rend);
}


//-------------------------------------------------------------------------------------------------
void Game::SpawnBox(const Vector3& extents, float inverseMass, const Vector3& position, const Vector3& rotationDegrees /*= Vector3::ZERO*/, const Vector3& velocity /*= Vector3::ZERO*/, const Vector3& angularVelocityDegrees /*= Vector3::ZERO*/, bool hasGravity /*= true*/)
{
	Entity* entity = new Entity();
	entity->transform.position = position;
	entity->transform.rotation = Quaternion::CreateFromEulerAnglesDegrees(rotationDegrees);
	entity->transform.scale = 2.f * extents;

	RigidBody* body = new RigidBody(&entity->transform);
	body->SetInverseMass(inverseMass);

	body->SetInertiaTensor_Box(extents);

	body->SetVelocityWs(velocity);
	body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
	body->SetAffectedByGravity(hasGravity);

	entity->rigidBody = body;
	entity->collider = new BoxCollider(entity, OBB3(Vector3::ZERO, Vector3(0.5f), Quaternion::IDENTITY));

	m_collisionScene->AddEntity(entity);
	m_physicsScene->AddRigidbody(body);
	m_entities.push_back(entity);

	Mesh* mesh = g_resourceSystem->CreateOrGetMesh("unit_cube");
	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/dot3.material");

	Renderable rend;
	rend.SetModelMatrix(entity->transform.GetModelMatrix());
	rend.AddDraw(mesh, material);
	m_renderScene->AddRenderable(entity->GetId(), rend);
}


//-------------------------------------------------------------------------------------------------
// Updates each entity's renderable to match the entity's updated transform
void Game::UpdateRenderables()
{
	for (Entity* entity : m_entities)
	{
		if (entity == m_player)
			continue;

		if (entity->collider->IsOfType<CapsuleCollider>())
		{
			Renderable* rend = m_renderScene->GetRenderable(entity->GetId());
			rend->SetModelMatrix(Matrix4::MakeModelMatrix(entity->transform.position, entity->transform.rotation, Vector3::ONES));
		}
		else if (entity->collider->IsOfType<HalfSpaceCollider>() || entity->collider->IsOfType<PlaneCollider>())
		{
			Renderable* rend = m_renderScene->GetRenderable(entity->GetId());
			Vector3 existngScale = Matrix4::ExtractScale(rend->GetModelMatrix());
			Matrix4 newModelMatrix = Matrix4::MakeModelMatrix(entity->transform.position, entity->transform.rotation, existngScale);
			rend->SetModelMatrix(newModelMatrix);
		}
		else
		{
			Renderable* rend = m_renderScene->GetRenderable(entity->GetId());
			rend->SetModelMatrix(entity->transform.GetLocalToWorldMatrix());
		}
	}
}


//-------------------------------------------------------------------------------------------------
void Game::SpawnSphere(float radius, float inverseMass, const Vector3& position, const Vector3& rotationDegrees /*= Vector3::ZERO*/, const Vector3& velocity /*= Vector3::ZERO*/, const Vector3& angularVelocityDegrees /*= Vector3::ZERO*/, bool hasGravity /*= true*/)
{
	Entity* entity = new Entity();
	entity->transform.position = position;
	entity->transform.rotation = Quaternion::CreateFromEulerAnglesDegrees(rotationDegrees);
	entity->transform.scale = Vector3(radius);

	RigidBody* body = new RigidBody(&entity->transform);
	body->SetInverseMass(inverseMass);
	body->SetInertiaTensor_Sphere(radius);
	body->SetVelocityWs(velocity);
	body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
	body->SetAffectedByGravity(hasGravity);

	entity->rigidBody = body;
	entity->collider = new SphereCollider(entity, Sphere3D(Vector3::ZERO, 1.0f));

	m_collisionScene->AddEntity(entity);
	m_physicsScene->AddRigidbody(body);
	m_entities.push_back(entity);

	Mesh* mesh = g_resourceSystem->CreateOrGetMesh("unit_sphere");
	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/dot3.material");

	Renderable rend;
	rend.SetModelMatrix(entity->transform.GetModelMatrix());
	rend.AddDraw(mesh, material);
	m_renderScene->AddRenderable(entity->GetId(), rend);
}


//-------------------------------------------------------------------------------------------------
// Spawns a duck.....no a cylinder actually
void Game::SpawnCylinder(float height, float radius, float inverseMass, const Vector3& position, const Vector3& rotationDegrees /*= Vector3::ZERO*/, const Vector3& velocity /*= Vector3::ZERO*/, const Vector3& angularVelocityDegrees /*= Vector3::ZERO*/, bool hasGravity /*= true*/)
{
	Entity* entity = new Entity();
	entity->transform.position = position;
	entity->transform.rotation = Quaternion::CreateFromEulerAnglesDegrees(rotationDegrees);
	entity->transform.scale = Vector3(radius, height, radius);

	//if (inverseMass > 0.f)
	//{
		RigidBody* body = new RigidBody(&entity->transform);
		body->SetInverseMass(inverseMass);
		body->SetInertiaTensor_Cylinder(height, radius);
		body->SetVelocityWs(velocity);
		body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
		body->SetAffectedByGravity(hasGravity);

		entity->rigidBody = body;
		m_physicsScene->AddRigidbody(body);
	//}

	entity->collider = new CylinderCollider(entity, Cylinder3D(Vector3(0.f, -0.5f, 0.f), Vector3(0.f, 0.5f, 0.f), 1.0f));

	m_collisionScene->AddEntity(entity);
	m_entities.push_back(entity);

	Mesh* cylMesh = g_resourceSystem->CreateOrGetMesh("cylinder");
	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/dot3.material");

	Renderable rend;
	rend.SetModelMatrix(entity->transform.GetModelMatrix());
	rend.AddDraw(cylMesh, material, Matrix4::IDENTITY);
	m_renderScene->AddRenderable(entity->GetId(), rend);
}


//-------------------------------------------------------------------------------------------------
// Spawns the entity used to represent the ground
void Game::SpawnGround()
{
	m_ground = new Entity();
	m_ground->collider = new HalfSpaceCollider(m_ground, Plane3(Vector3::Y_AXIS, Vector3::ZERO));
	//m_ground->collider = new PlaneCollider(m_ground, Plane3(Vector3::Y_AXIS, Vector3::ZERO));
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
void Game::SpawnPolygon(float inverseMass, const Vector3& position, const Vector3& rotationDegrees /*= Vector3::ZERO*/, const Vector3& velocity /*= Vector3::ZERO*/, const Vector3& angularVelocityDegrees /*= Vector3::ZERO*/, bool hasGravity /*= true*/)
{
	Entity* entity = new Entity();
	entity->transform.position = position;
	entity->transform.rotation = Quaternion::CreateFromEulerAnglesDegrees(rotationDegrees);

	RigidBody* body = new RigidBody(&entity->transform);
	body->SetInverseMass(inverseMass);

	body->SetInertiaTensor_Polygon(m_poly);

	body->SetVelocityWs(velocity);
	body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
	body->SetAffectedByGravity(hasGravity);

	entity->rigidBody = body;
	entity->collider = new PolygonCollider(entity, m_poly);

	m_collisionScene->AddEntity(entity);
	m_physicsScene->AddRigidbody(body);
	m_entities.push_back(entity);

	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/surface_normal.material");

	Renderable rend;
	rend.SetModelMatrix(entity->transform.GetModelMatrix());
	rend.AddDraw(m_polyMesh, material);
	m_renderScene->AddRenderable(entity->GetId(), rend);
}
