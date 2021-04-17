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
#include "Engine/Collision/3D/Collider3d.h"
#include "Engine/Collision/3D/CollisionSystem3d.h"
#include "Engine/Framework/DevConsole.h"
#include "Engine/Framework/EngineCommon.h"
#include "Engine/Framework/Entity.h"
#include "Engine/Framework/GameObject.h"
#include "Engine/Framework/Window.h"
#include "Engine/IO/Image.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Job/JobSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/OBB2.h"
#include "Engine/Math/OBB3.h"
#include "Engine/Math/Polygon3d.h"
#include "Engine/Physics/3D/PhysicsSystem3D.h"
#include "Engine/Render/Camera/Camera.h"
#include "Engine/Render/Core/Renderable.h"
#include "Engine/Render/Core/RenderContext.h"
#include "Engine/Render/Font/Font.h"
#include "Engine/Render/Font/FontAtlas.h"
#include "Engine/Render/Font/FontLoader.h"
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
#include "Engine/Utility/StringId.h"
#include "Engine/UI/Canvas.h"
#include "Engine/UI/UIPanel.h"
#include "Engine/UI/UIText.h"
#include "Engine/Voxel/QEFLoader.h"
#include "Engine/Physics/3D/Arbiter3D.h"
#include "Engine/Physics/3D/RigidBody3D.h"

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
	SetupObjects();
}


//-------------------------------------------------------------------------------------------------
Game::~Game()
{
	m_collisionSystem->RemoveEntity(m_entity1);
	m_collisionSystem->RemoveEntity(m_entity2);

	SAFE_DELETE(m_entity2);
	SAFE_DELETE(m_entity1);
	SAFE_DELETE(m_physicsSystem);
	SAFE_DELETE(m_collisionSystem);

	SAFE_DELETE(m_material);
	SAFE_DELETE(m_shader);
	SAFE_DELETE(m_texture);
	SAFE_DELETE(m_image);
	SAFE_DELETE(m_uiCamera);
	SAFE_DELETE(m_gameCamera);

	SAFE_DELETE(m_timer);
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
int SolveEdgeSATGame(const Polygon3d* a, const Polygon3d* b, std::vector<AxisResult>& out_results)
{
	out_results.clear();

	UniqueHalfEdgeIterator edgeAIter(*a);
	const HalfEdge* edgeA = edgeAIter.GetNext();

	float maxDistance = 0.f;
	bool firstIteration = true;
	int bestIndex = -1;

	while (edgeA != nullptr)
	{
		Vector3 directionA = a->GetEdgeDirection(edgeA);
		Vector3 outwardDirA = a->GetVertexPosition(edgeA->m_vertexIndex) - a->GetCenter();

		UniqueHalfEdgeIterator edgeBIter(*b);
		const HalfEdge* edgeB = edgeBIter.GetNext();

		while (edgeB != nullptr)
		{
			Vector3 aStart = a->GetVertexPosition(edgeA->m_vertexIndex);
			Vector3 aEnd = a->GetVertexPosition(a->GetEdge(edgeA->m_nextEdgeIndex)->m_vertexIndex);
			Vector3 bStart = b->GetVertexPosition(edgeB->m_vertexIndex);
			Vector3 bEnd = b->GetVertexPosition(b->GetEdge(edgeB->m_nextEdgeIndex)->m_vertexIndex);

			Vector3 aDir = aEnd - aStart;
			Vector3 bDir = bEnd - bStart;

			Vector3 directionB = b->GetEdgeDirection(edgeB);

			Vector3 normal = CrossProduct(directionA, directionB);

			if (AreMostlyEqual(normal.GetLengthSquared(), 0.f))
			{
				edgeB = edgeBIter.GetNext();
				continue;
			}
			else
			{
				normal.Normalize();
			}

			// Ensure the normal points away from A
			if (AreMostlyEqual(DotProduct(normal, outwardDirA), 0.f))
			{
				edgeB = edgeBIter.GetNext();
				continue;
			}
			else if (DotProduct(normal, outwardDirA) < 0.f)
			{
				normal *= -1.0f;
			}

			// Make a plane on the edge of A facing outward from A
			Plane3 plane(normal, a->GetVertexPosition(edgeA->m_vertexIndex));

			// Get the vertex in B that would be furthest against the normal (if anything will be behind this plane, it would be that point)
			Vector3 supportB;
			b->GetSupportPoint(-1.0f * normal, supportB);
			float distance = plane.GetDistanceFromPlane(supportB);

			AxisResult result;
			result.m_distance = distance;
			result.m_plane = plane;
			result.m_edgeA = edgeA;
			result.m_edgeB = edgeB;
			result.m_supportPoint = supportB;
			result.m_distanceCenterToPlane = plane.GetDistanceFromPlane(a->GetCenter());

			out_results.push_back(result);

			if (firstIteration || distance > maxDistance)
			{
				maxDistance = distance;
				bestIndex = (int)out_results.size() - 1;
				firstIteration = false;
			}

			edgeB = edgeBIter.GetNext();
		}

		edgeA = edgeAIter.GetNext();
	}

	return bestIndex;
}


std::vector<AxisResult> g_results;
int g_index = 0;
int g_bestIndex = 0;
//-------------------------------------------------------------------------------------------------
void Game::ProcessInput()
{
	const float deltaSeconds = m_gameClock->GetDeltaSeconds();

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_LEFT_ARROW))
	{
		m_entity2->transform.position.x += -1.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_RIGHT_ARROW))
	{
		m_entity2->transform.position.x += 1.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_DOWN_ARROW))
	{
		m_entity2->transform.position.z += -1.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_UP_ARROW))
	{
		m_entity2->transform.position.z += 1.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed('K'))
	{
		m_entity2->transform.position.y += -1.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed('I'))
	{
		m_entity2->transform.position.y += 1.f * deltaSeconds;
	}

	if (g_inputSystem->WasKeyJustPressed(InputSystem::KEYBOARD_F1))
	{
		Mouse& mouse = InputSystem::GetMouse();
		mouse.ShowMouseCursor(!mouse.IsCursorShown());
		mouse.LockCursorToClient(!mouse.IsCursorLocked());
		mouse.SetCursorMode(mouse.GetCursorMode() == CURSORMODE_RELATIVE ? CURSORMODE_ABSOLUTE : CURSORMODE_RELATIVE);
	}

	
	if (g_inputSystem->WasKeyJustPressed('G'))
	{
		g_index = Clamp(g_index - 1, 0, 100);
	}

	if (g_inputSystem->WasKeyJustPressed('H'))
	{
		g_index = Clamp(g_index + 1, 0, (int)g_results.size() - 1);
	}

	if (g_inputSystem->WasKeyJustPressed('J'))
	{
		g_index = g_bestIndex;
	}

	// Update da camera

	// Translating the camera
	Vector3 translationOffset = Vector3::ZERO;
	if (g_inputSystem->IsKeyPressed('W')) { translationOffset.z += 1.f; }		// Forward
	if (g_inputSystem->IsKeyPressed('S')) { translationOffset.z -= 1.f; }		// Left
	if (g_inputSystem->IsKeyPressed('A')) { translationOffset.x -= 1.f; }		// Back
	if (g_inputSystem->IsKeyPressed('D')) { translationOffset.x += 1.f; }		// Right
	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SPACEBAR)) { translationOffset.y += 1.f; }		// Up
	if (g_inputSystem->IsKeyPressed('X')) { translationOffset.y -= 1.f; }		// Down

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SHIFT))
	{
		translationOffset *= 50.f;
	}

	translationOffset *= deltaSeconds * 10.f;

	m_gameCamera->Translate(translationOffset);

	// Rotating the camera
	Mouse& mouse = InputSystem::GetMouse();
	IntVector2 mouseDelta = mouse.GetMouseDelta();

	Vector2 rotationOffset = Vector2((float)mouseDelta.y, (float)mouseDelta.x);
	Vector3 deltaRotation = Vector3(rotationOffset.x * 90.f * deltaSeconds, rotationOffset.y * 90.f * deltaSeconds, 0.f);
	m_gameCamera->SetRotation(m_gameCamera->GetRotation() + deltaRotation);

	if (g_inputSystem->WasKeyJustPressed(InputSystem::KEYBOARD_F9))
	{
		CreateDirectoryA("Data/Screenshots", NULL);
		g_renderContext->SaveTextureToImage(g_renderContext->GetDefaultRenderTarget(), "Data/Screenshots/Latest.png");
		g_renderContext->SaveTextureToImage(g_renderContext->GetDefaultRenderTarget(), Stringf("Data/Screenshots/Screenshot_%s.png", GetFormattedSystemDateAndTime().c_str()).c_str());
	}

	if (g_inputSystem->WasKeyJustPressed('B'))
	{
		m_entity1->transform.position = Vector3(0.f, 2.f, 0.f);
		m_entity2->transform.position = Vector3(0.5f, 0.f, 0.f);
		//m_entity1->GetRigidBody()->AddTorque(Vector3(50.f, 50.f, 0.f));
		//m_entity2->GetRigidBody()->AddTorque(Vector3(50.f, 50.f, 0.f));
	}

	if (g_inputSystem->WasKeyJustPressed('N'))
	{
		m_doPhysics = !m_doPhysics;
	}
}


//-------------------------------------------------------------------------------------------------
void Game::Update()
{
	//m_entity1->GetCollider()->GetAsType<PolytopeCollider3d>()->GenerateWorldShape();
	//m_entity2->GetCollider()->GetAsType<PolytopeCollider3d>()->GenerateWorldShape();
	//const Polygon3d* shape1 = m_entity1->GetCollider()->GetAsType<PolytopeCollider3d>()->GetWorldShape();
	//const Polygon3d* shape2 = m_entity2->GetCollider()->GetAsType<PolytopeCollider3d>()->GetWorldShape();
	//
	//g_bestIndex = SolveEdgeSATGame(shape1, shape2, g_results);
	const float deltaSeconds = m_gameClock->GetDeltaSeconds();
	//m_entity1->transform.Rotate(Vector3(0.f, 90.f * deltaSeconds, 0.f));

	m_collisionSystem->PerformBroadPhase();
	m_collisionSystem->PerformNarrowPhase();

	if (m_doPhysics)
	{
		m_physicsSystem->FrameStep(deltaSeconds, m_collisionSystem);
	}

	const ContactManifold3d* man = m_collisionSystem->GetManifoldForColliders(m_entity1->GetCollider(), m_entity2->GetCollider());
	if (man)
	{
		//float pushDir = (man->GetReferenceEntity() == m_entity1 ? 1.0f : -1.0f);
		//m_entity2->transform.position += pushDir * man->GetBroadphaseResult().m_direction * man->GetBroadphaseResult().m_penetration;
	}
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

	Vector3 rot = m_entity1->transform.GetWorldRotationDegrees();
	Quaternion quat = Quaternion::FromEulerAngles(rot);

	// Render selected result
	const Polygon3d* shape1 = m_entity1->GetCollider()->GetAsType<PolytopeCollider3d>()->GetWorldShape();
	const Polygon3d* shape2 = m_entity2->GetCollider()->GetAsType<PolytopeCollider3d>()->GetWorldShape();

	//AxisResult& result = g_results[g_index];
	//g_renderContext->DrawLine3D(shape1->GetVertexPosition(result.m_edgeA->m_vertexIndex), shape1->GetVertexPosition(shape1->GetEdge(result.m_edgeA->m_nextEdgeIndex)->m_vertexIndex), m_material, Rgba::MAGENTA);
	//g_renderContext->DrawLine3D(shape2->GetVertexPosition(result.m_edgeB->m_vertexIndex), shape2->GetVertexPosition(shape2->GetEdge(result.m_edgeB->m_nextEdgeIndex)->m_vertexIndex), m_material, Rgba::CYAN);
	//g_renderContext->DrawPlane3(result.m_plane, m_material, Rgba(255, 255, 255, 100));
	//g_renderContext->DrawPoint3D(result.m_supportPoint, 0.05f, m_material, Rgba::YELLOW);
	//g_renderContext->DrawLine3D(result.m_supportPoint, result.m_supportPoint + -1.0f * result.m_plane.GetNormal() * result.m_distance, m_material, Rgba::ORANGE);

	//const ContactManifold3d* man = m_collisionSystem->GetManifoldForColliders(m_entity1->GetCollider(), m_entity2->GetCollider());
	//if (man)
	//{
	g_renderContext->DrawTransform(m_entity1->transform, m_material, 1.f);
	g_renderContext->DrawTransform(m_entity2->transform, m_material, 1.f);

	m_entity1->GetRigidBody()->DebugRender(m_material, Rgba::RED);
	m_entity2->GetRigidBody()->DebugRender(m_material, Rgba::BLUE);

	for (int i = 0; i < 10; ++i)
	{
		m_entities[i]->GetRigidBody()->DebugRender(m_material, Rgba(10 * i, 20 * i, 255 - 10 * i, 255));
	}

	//m_shader->SetFillMode(FILL_MODE_SOLID);
	//man->DebugRender(m_material);
	//m_shader->SetFillMode(FILL_MODE_WIREFRAME);
		//g_renderContext->DrawLine3D(m_entity2->transform.position, m_entity2->transform.position + man->GetBroadphaseResult().m_direction * man->GetBroadphaseResult().m_penetration, m_material, Rgba::RED);
	//}
	//else
	//{
	//	Renderable rend;
	//	rend.SetRenderableMatrix(m_entity1->transform.GetLocalToWorldMatrix());
	//	rend.AddDraw(m_mesh, m_material);
	//	g_renderContext->DrawRenderable(rend);
	//	
	//	rend.SetRenderableMatrix(m_entity2->transform.GetLocalToWorldMatrix());
	//	g_renderContext->DrawRenderable(rend);
	//}

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
	m_timer = new FrameTimer(m_gameClock);
	m_timer->SetInterval(0.01f);

	m_collisionSystem = new CollisionSystem3d();
	m_physicsSystem = new PhysicsSystem3D();
}


//-------------------------------------------------------------------------------------------------
void Game::SetupRendering()
{
	// Cameras
	m_gameCamera = new Camera();
	m_gameCamera->SetProjectionPerspective(90.f, 0.1f, 100.f);
	m_gameCamera->LookAt(Vector3(0.f, 0.f, -10.f), Vector3(0.f, 0.f, 0.f));
	m_gameCamera->SetDepthTarget(g_renderContext->GetDefaultDepthStencilTarget(), false);

	m_uiCamera = new Camera();
	m_uiCamera->SetProjectionOrthographic((float)g_window->GetClientPixelHeight(), g_window->GetClientAspect());

	// Shader
	m_shader = new Shader();
	m_shader->CreateFromFile("Data/Shader/test.shader");
	m_shader->SetBlend(BLEND_PRESET_ALPHA);
	m_shader->SetFillMode(FILL_MODE_WIREFRAME);

	// Texture
	m_image = new Image(IntVector2(2));

	m_texture = new Texture2D();
	m_texture->CreateFromImage(*m_image, TEXTURE_USAGE_SHADER_RESOURCE_BIT, GPU_MEMORY_USAGE_STATIC);
	m_textureView = m_texture->CreateOrGetShaderResourceView();

	// Combine into default material
	m_material = new Material();
	m_material->SetShader(m_shader);
	m_material->SetAlbedoTextureView(m_textureView);

	MeshBuilder mb;
	mb.BeginBuilding(true);
	mb.PushCube(Vector3::ZERO, Vector3(1.f), AABB2::ZERO_TO_ONE, AABB2::ZERO_TO_ONE, AABB2::ZERO_TO_ONE, Rgba::BLUE);
	mb.FinishBuilding();

	m_mesh = mb.CreateMesh<Vertex3D_PCU>();
}


//-------------------------------------------------------------------------------------------------
void Game::SetupObjects()
{
	m_entity1 = new Entity();
	m_entity2 = new Entity();

	OBB3 colliderBounds = OBB3(Vector3::ZERO, Vector3(0.5f), Vector3::ZERO);
	OBB3 groundBounds = OBB3(Vector3::ZERO, Vector3(100.f, 0.1f, 100.f), Vector3::ZERO);

	Polygon3d cone;
	const int numVertsInCircle = 8;

	cone.AddVertex(Vector3(0.f, 1.f, 0.f));
	float degPerI = 360.f / (float)numVertsInCircle;
	float radius = 1.f;

	std::vector<int> faceIndices;

	for (int i = 0; i < numVertsInCircle; ++i)
	{
		float degrees = degPerI * i;
		cone.AddVertex(radius * Vector3(CosDegrees(degrees), -1.f, SinDegrees(degrees)));
		faceIndices.push_back(i + 1);
	}

	cone.AddFace(faceIndices);

	for (int i = 1; i <= numVertsInCircle; ++i)
	{
		faceIndices.clear();
		faceIndices.push_back(0);
		faceIndices.push_back(((i == numVertsInCircle ? 1 : i + 1)));
		faceIndices.push_back(i);

		cone.AddFace(faceIndices);
	}


	m_collisionSystem->AddEntity(m_entity1, &cone);
	m_collisionSystem->AddEntity(m_entity2, groundBounds);

	m_physicsSystem->AddEntity(m_entity1);
	m_physicsSystem->AddEntity(m_entity2);

	m_entity1->GetRigidBody()->SetMassProperties(1.f);
	m_entity2->GetRigidBody()->SetMassProperties(FLT_MAX);
	m_entity2->GetRigidBody()->SetAffectedByGravity(false);

	m_entity1->transform.position = Vector3(0.f, 2.f, 0.f);
	m_entity2->transform.position = Vector3(0.f, 0.f, 0.f);
	//m_entity1->transform.scale = Vector3(1.f);
	//m_entity2->transform.position = Vector3(-1.32701576f, 0.f, -0.746994615f);
	m_entity1->transform.SetRotation(Vector3(30.f, 45.f, 0.f));
	//m_entity2->transform.SetRotation(Vector3(0.f, 45.f, 0.f));

	for (int i = 0; i < 10; ++i)
	{
		m_entities[i] = new Entity();
		m_collisionSystem->AddEntity(m_entities[i], colliderBounds);
		m_physicsSystem->AddEntity(m_entities[i]);

		m_entities[i]->transform.position = Vector3(0.f, 4.f + 2.f * (float)i, 0.f);
		m_entities[i]->GetRigidBody()->SetMassProperties(1.f);
	}
}
