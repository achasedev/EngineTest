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
#include "Engine/Math/Triangle2.h"
#include "Engine/Math/GJK.h"
#include "Engine/Math/LineSegment2.h"

//-------------------------------------------------------------------------------------------------
Game::Game()
{
	SetupFramework();
	SetupRendering();
	SpawnEntities();

	LineSegment2 a(Vector2(1.f, 1.f), Vector2(2.f, 2.f));
	LineSegment2 b(Vector2(2.f, 3.f), Vector2(4.f, 1.f));

	bool intersect = DoLineSegmentsIntersect(a, b);

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
		const int numTypes = 6;
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

	if (m_spawnType == 2)
	{
		Transform test;
		test.position = m_gameCamera->GetPosition() + 2.f * m_gameCamera->GetForwardVector();
		test.scale = Vector3(0.25f, 0.5f, 0.25f);

		Vector3 start = test.TransformPosition(Vector3(0.f, -0.5f, 0.f));
		Vector3 end = test.TransformPosition(Vector3(0.f, 0.5f, 0.f));
		Capsule3 cap = Capsule3(start, end, 0.25f);
		
		DebugRenderOptions options;
		options.m_startColor = Rgba::RED;
		options.m_debugRenderMode = DEBUG_RENDER_MODE_XRAY;
		options.m_fillMode = FILL_MODE_WIREFRAME;
		options.m_lifetime = 0.f;
		
		DebugDrawCapsule(cap, options);
	}

	if (mouse.WasButtonJustPressed(MOUSEBUTTON_LEFT))
	{
		Vector3 spawnPosition = m_gameCamera->GetPosition() + 2.f * m_gameCamera->GetForwardVector();
		Vector3 velocity = 10.f * m_gameCamera->GetForwardVector();
		float mass = 5.f;

		switch (m_spawnType)
		{
		case 0:
			SpawnBox(Vector3(1.f, 1.f, 1.5f), 1.0f / (100.f * mass), spawnPosition, Vector3::ZERO, velocity, Vector3::ZERO);
			break;
		case 1:
			SpawnSphere(0.5f, 1.f / mass, spawnPosition, Vector3::ZERO, velocity);
			break;
		case 2:
			SpawnCapsule(0.5f, 0.25f, 1.f / mass, spawnPosition, Vector3::ZERO, Vector3::ZERO);
			break;
		case 3:
			SpawnCylinder(0.5f, 1.25f, 1.f / mass, spawnPosition, Vector3(0.f, 0.f, 0.f), velocity);
			break;
		case 4:
			SpawnPolygon(0.f, spawnPosition, Vector3::ZERO, velocity, Vector3::ZERO);
			break;
		case 5:
			SpawnPolygon2(1.f / mass, spawnPosition, Vector3::ZERO, velocity, Vector3::ZERO);
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
	case 5:
		ConsolePrintf(Rgba::CYAN, 0.f, "Spawn Type: Polygon 2");
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
	
	m_poly[0].AddVertex(Vector3(-1.f, 1.f, -1.f));
	m_poly[0].AddVertex(Vector3(-1.f, 2.f, -1.f));
	m_poly[0].AddVertex(Vector3(1.f, 1.f, -1.f));
	m_poly[0].AddVertex(Vector3(1.f, 1.f, 1.f));
	m_poly[0].AddVertex(Vector3(-1.f, 2.f, +1.f));
	m_poly[0].AddVertex(Vector3(-1.f, 1.f, +2.f));

	std::vector<int> face0{ 0, 1, 2 };
	std::vector<int> face1{ 3, 4, 5 };
	std::vector<int> face2{ 5, 4, 1, 0 };
	std::vector<int> face3{ 5, 0, 2, 3 };
	std::vector<int> face4{ 2, 1, 4, 3 };

	m_poly[0].AddFace(face0);
	m_poly[0].AddFace(face1);
	m_poly[0].AddFace(face2);
	m_poly[0].AddFace(face3);
	m_poly[0].AddFace(face4);

	MeshBuilder mb;
	mb.BeginBuilding(TOPOLOGY_TRIANGLE_LIST, true);
	mb.PushPolyhedron(m_poly[0]);
	mb.GenerateFlatNormals();
	mb.FinishBuilding();

	m_polyMesh[0] = mb.CreateMesh<VertexLit>();

	m_poly[1].AddVertex(Vector3(0.f, 2.f, 0.f));
	int numSegments = 10;
	float anglePerSegment = (360.f / (float)numSegments);
	float radius = 1.f;
	std::vector<int> bottomFaceIndices;

	for (int i = 0; i < numSegments; ++i)
	{
		float angle = anglePerSegment * (float)i;
		Vector3 pos = Vector3(radius * CosDegrees(angle), 0.f, radius * SinDegrees(angle));

		m_poly[1].AddVertex(pos);
		bottomFaceIndices.push_back(i + 1);
	}

	m_poly[1].AddFace(bottomFaceIndices);
	for (int i = 1; i <= numSegments; ++i)
	{
		int next = (i == numSegments ? 1 : i + 1);
		std::vector<int> indices = { next, i, 0 };

		m_poly[1].AddFace(indices);
	}

	mb.Clear();
	mb.BeginBuilding(TOPOLOGY_TRIANGLE_LIST, true);
	mb.PushPolyhedron(m_poly[1]);
	mb.GenerateFlatNormals();
	mb.FinishBuilding();
	m_polyMesh[1] = mb.CreateMesh<VertexLit>();

	OBB3 box = OBB3(Vector3(0.f, 1.f, 0.f), Vector3::ONES, Quaternion::IDENTITY);
	m_poly[2] = Polyhedron(box);
	mb.Clear();
	mb.BeginBuilding(TOPOLOGY_TRIANGLE_LIST, true);
	mb.PushPolyhedron(m_poly[2]);
	mb.GenerateFlatNormals();
	mb.FinishBuilding();
	m_polyMesh[2] = mb.CreateMesh<VertexLit>();

	m_poly[3].AddVertex(Vector3(+0.00000000, +0.00000000 + 1.f, +1.00000000));
	m_poly[3].AddVertex(Vector3(+0.00000000, +0.00000000 + 1.f, -1.00000000));
	m_poly[3].AddVertex(Vector3(+0.89442719, +0.00000000 + 1.f, +0.44721360));
	m_poly[3].AddVertex(Vector3(+0.27639320, +0.85065081 + 1.f, +0.44721360));
	m_poly[3].AddVertex(Vector3(-0.72360680, +0.52573111 + 1.f, +0.44721360));
	m_poly[3].AddVertex(Vector3(-0.72360680, -0.52573111 + 1.f, +0.44721360));
	m_poly[3].AddVertex(Vector3(+0.27639320, -0.85065081 + 1.f, +0.44721360));
	m_poly[3].AddVertex(Vector3(+0.72360680, +0.52573111 + 1.f, -0.44721360));
	m_poly[3].AddVertex(Vector3(-0.27639320, +0.85065081 + 1.f, -0.44721360));
	m_poly[3].AddVertex(Vector3(-0.89442719, +0.00000000 + 1.f, -0.44721360));
	m_poly[3].AddVertex(Vector3(-0.27639320, -0.85065081 + 1.f, -0.44721360));
	m_poly[3].AddVertex(Vector3(+0.72360680, -0.52573111 + 1.f, -0.44721360));

	std::vector<int> icosaVerts0 = { 6 ,     11,     2 };
	std::vector<int> icosaVerts1 = { 3 ,     2 ,     7 };
	std::vector<int> icosaVerts2 = { 7 ,     2 ,     11};
	std::vector<int> icosaVerts3 = { 0 ,     2 ,     3 };
	std::vector<int> icosaVerts4 = { 0 ,     6 ,     2 };
	std::vector<int> icosaVerts5 = { 10,     1 ,     11};
	std::vector<int> icosaVerts6 = { 1 ,     7 ,     11};
	std::vector<int> icosaVerts7 = { 10,     11,     6 };
	std::vector<int> icosaVerts8 = { 8 ,     7 ,     1 };
	std::vector<int> icosaVerts9 = { 8 ,     3 ,     7 };
	std::vector<int> icosaVerts10 = { 5 ,     10,     6 };
	std::vector<int> icosaVerts11 = { 5 ,     6 ,     0 };
	std::vector<int> icosaVerts12 = { 4 ,     3 ,     8 };
	std::vector<int> icosaVerts13 = { 4 ,     0 ,     3 };
	std::vector<int> icosaVerts14 = { 9 ,     8 ,     1 };
	std::vector<int> icosaVerts15 = { 9 ,     1 ,     10};
	std::vector<int> icosaVerts16 = { 4 ,     5 ,     0 };
	std::vector<int> icosaVerts17 = { 9 ,     10,     5 };
	std::vector<int> icosaVerts18 = { 9 ,     5 ,     4 };
	std::vector<int> icosaVerts19 = { 9 ,     4 ,     8 };

	m_poly[3].AddFace(icosaVerts0);
	m_poly[3].AddFace(icosaVerts1);
	m_poly[3].AddFace(icosaVerts2);
	m_poly[3].AddFace(icosaVerts3);
	m_poly[3].AddFace(icosaVerts4);
	m_poly[3].AddFace(icosaVerts5);
	m_poly[3].AddFace(icosaVerts6);
	m_poly[3].AddFace(icosaVerts7);
	m_poly[3].AddFace(icosaVerts8);
	m_poly[3].AddFace(icosaVerts9);
	m_poly[3].AddFace(icosaVerts10);
	m_poly[3].AddFace(icosaVerts11);
	m_poly[3].AddFace(icosaVerts12);
	m_poly[3].AddFace(icosaVerts13);
	m_poly[3].AddFace(icosaVerts14);
	m_poly[3].AddFace(icosaVerts15);
	m_poly[3].AddFace(icosaVerts16);
	m_poly[3].AddFace(icosaVerts17);
	m_poly[3].AddFace(icosaVerts18);
	m_poly[3].AddFace(icosaVerts19);
	mb.Clear();
	mb.BeginBuilding(TOPOLOGY_TRIANGLE_LIST, true);
	mb.PushPolyhedron(m_poly[3]);
	mb.GenerateFlatNormals();
	mb.FinishBuilding();
	m_polyMesh[3] = mb.CreateMesh<VertexLit>();

	for (int i = 0; i < 4; ++i)
	{
		if (i == 2)
			continue;

		m_poly[i].GenerateHalfEdgeStructure();
	}

	Triangle3 tri(Vector3(-1.f, 1.f, 0.f), Vector3(0.f, 1.f, 1.f), Vector3(1.f, 1.f, 0.f));
	DebugRenderOptions options;
	options.m_startColor = Rgba::BLUE;

	//DebugDrawTriangle3(tri, options);

	Polygon3 poly;
	poly.m_vertices.push_back(Vector3(-4.f, 1.f, -1.f));
	poly.m_vertices.push_back(Vector3(-5.f, 2.f, 0.f));
	poly.m_vertices.push_back(Vector3(0.f, 3.f, 1.f));
	poly.m_vertices.push_back(Vector3(2.f, 2.f, 0.f));
	poly.m_vertices.push_back(Vector3(2.f, 1.f, -1.f));

	for (int i = 0; i < poly.GetNumVertices(); ++i)
	{
		DebugRenderOptions options2;
		options2.m_startColor = Rgba::RED;

		//DebugDrawSphere(poly.GetVertex(i), 0.1f, options2);
	}

	//DebugDrawPolygon(poly, options);

	//Tetrahedron tetra(Vector3(-4.f, 2.f, 0.f), Vector3(1.f, 2.f, -2.f), Vector3(0.f, 2.f, 1.f), Vector3(0.f, 5.f, 0.f));

	//mb.Clear();
	//mb.BeginBuilding(TOPOLOGY_TRIANGLE_LIST, true);
	//mb.PushTetrahedron(tetra);
	//mb.FinishBuilding();

	//Mesh* mesh = mb.CreateMesh<VertexLit>();

	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/surface_normal.material");
	Renderable rend;
	rend.AddDraw(m_polyMesh[3], material);

	m_renderScene->AddRenderable(400, rend);
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

	Triangle3 tri(Vector3(-1.f, 1.f, 0.f), Vector3(0.f, 1.f, 1.f), Vector3(1.f, 1.f, 0.f));

	Polygon3 poly;
	poly.m_vertices.push_back(Vector3(-4.f, 1.f, -1.f));
	poly.m_vertices.push_back(Vector3(-5.f, 2.f, 0.f));
	poly.m_vertices.push_back(Vector3(0.f, 3.f, 1.f));
	poly.m_vertices.push_back(Vector3(2.f, 2.f, 0.f));
	poly.m_vertices.push_back(Vector3(2.f, 1.f, -1.f));

	//Vector3 closestPt;
	//float dist = FindNearestPoint(m_gameCamera->GetPosition() + m_gameCamera->GetForwardVector(), tri, closestPt);
	////float dist = FindNearestPoint(m_gameCamera->GetPosition() + m_gameCamera->GetForwardVector(), poly, closestPt);
	//ConsolePrintf("Dist: %.2f", dist);

	//DebugRenderOptions options;
	//options.m_startColor = Rgba::RED;
	//options.m_lifetime = 0.f;

	//DebugDrawSphere(m_gameCamera->GetPosition() + m_gameCamera->GetForwardVector(), 0.1f, options);
	//DebugDrawSphere(closestPt, 0.1f, options);
	//DebugDrawLine(m_gameCamera->GetPosition() + m_gameCamera->GetForwardVector(), closestPt, options);

	Tetrahedron tetra(Vector3(-4.f, 2.f, 0.f), Vector3(1.f, 2.f, -2.f), Vector3(0.f, 2.f, 1.f), Vector3(0.f, 5.f, 0.f));
	Vector3 closestPt;
	//float dist = FindNearestPoint(m_gameCamera->GetPosition() + m_gameCamera->GetForwardVector(), tetra, closestPt);
	float angle = (360.f / 10.f) * 0.5f;
	//Vector3 input = Vector3(0.3f * CosDegrees(angle), 2.5f, 0.3f * SinDegrees(angle));
	Vector3 input = m_gameCamera->GetPosition() + m_gameCamera->GetForwardVector();
	LineSegment3 seg(input - Vector3(0.f, 0.5f, 0.f), input + Vector3(0.f, 0.5f, 0.5f));

	Vector3 closestOnLine, closestOnPoly;
	float dist = FindNearestPoints(seg, m_poly[3], closestOnLine, closestOnPoly);
	//float dist = FindNearestPoint(input, m_poly[0], closestPt);

	DebugRenderOptions options;
	options.m_startColor = Rgba::RED;
	options.m_lifetime = 0.f;

	//DebugDrawSphere(closestPt, 0.1f, options);
	//DebugDrawSphere(input, 0.1f, options);
	//DebugDrawLine(input, closestPt, options);

	DebugDrawSphere(closestOnLine, 0.1f, options);
	DebugDrawSphere(closestOnPoly, 0.1f, options);
	DebugDrawLine(closestOnLine, closestOnPoly, options);

	options.m_startColor = Rgba::YELLOW;

	DebugDrawSphere(seg.m_a, 0.1f, options);
	DebugDrawSphere(seg.m_b, 0.1f, options);
	DebugDrawLine(seg.m_a, seg.m_b, options);

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
	entity->collider = new CapsuleCollider(entity, Capsule3(Vector3(0.f, -0.5f, 0.f), Vector3(0.f, 0.5f, 0.f), 1.0f));

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
	entity->collider = new SphereCollider(entity, Sphere(Vector3::ZERO, 1.0f));

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

	entity->collider = new CylinderCollider(entity, Cylinder(Vector3(0.f, -0.5f, 0.f), Vector3(0.f, 0.5f, 0.f), 1.0f));

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

	int iPoly = 2;
	if (inverseMass > 0.f)
	{
		RigidBody* body = new RigidBody(&entity->transform);
		body->SetInverseMass(inverseMass);

		//int iPoly = GetRandomIntInRange(0, 3);
		body->SetInertiaTensor_Polygon(m_poly[iPoly]);

		body->SetVelocityWs(velocity);
		body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
		body->SetAffectedByGravity(hasGravity);
		entity->rigidBody = body;
		m_physicsScene->AddRigidbody(body);
	}

	entity->collider = new ConvexHullCollider(entity, m_poly[iPoly]);

	m_collisionScene->AddEntity(entity);
	m_entities.push_back(entity);

	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/surface_normal.material");

	Renderable rend;
	rend.SetModelMatrix(entity->transform.GetModelMatrix());
	rend.AddDraw(m_polyMesh[iPoly], material);
	m_renderScene->AddRenderable(entity->GetId(), rend);
}


//-------------------------------------------------------------------------------------------------
void Game::SpawnPolygon2(float inverseMass, const Vector3& position, const Vector3& rotationDegrees /*= Vector3::ZERO*/, const Vector3& velocity /*= Vector3::ZERO*/, const Vector3& angularVelocityDegrees /*= Vector3::ZERO*/, bool hasGravity /*= true*/)
{
	Entity* entity = new Entity();
	entity->transform.position = position;
	entity->transform.rotation = Quaternion::CreateFromEulerAnglesDegrees(rotationDegrees);

	int iPoly = 1;
	if (inverseMass > 0.f)
	{
		RigidBody* body = new RigidBody(&entity->transform);
		body->SetInverseMass(inverseMass);

		//int iPoly = GetRandomIntInRange(0, 3);
		body->SetInertiaTensor_Polygon(m_poly[iPoly]);

		body->SetVelocityWs(velocity);
		body->SetAngularVelocityDegreesWs(angularVelocityDegrees);
		body->SetAffectedByGravity(hasGravity);
		entity->rigidBody = body;
		m_physicsScene->AddRigidbody(body);
	}

	entity->collider = new ConvexHullCollider(entity, m_poly[iPoly]);

	m_collisionScene->AddEntity(entity);
	m_entities.push_back(entity);

	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/surface_normal.material");

	Renderable rend;
	rend.SetModelMatrix(entity->transform.GetModelMatrix());
	rend.AddDraw(m_polyMesh[iPoly], material);
	m_renderScene->AddRenderable(entity->GetId(), rend);
}
