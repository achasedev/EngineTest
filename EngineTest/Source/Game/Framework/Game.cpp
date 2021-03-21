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
#include "Engine/Math/Polygon3D.h"
#include "Engine/Physics/3D/Physics3D.h"
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


//-------------------------------------------------------------------------------------------------
void Game::ProcessInput()
{
	const float deltaSeconds = m_gameClock->GetDeltaSeconds();

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_LEFT_ARROW))
	{
		m_entity2->transform.position.x += -10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_RIGHT_ARROW))
	{
		m_entity2->transform.position.x += 10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_DOWN_ARROW))
	{
		m_entity2->transform.position.z += -10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_UP_ARROW))
	{
		m_entity2->transform.position.z += 10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed('K'))
	{
		m_entity2->transform.position.y += -10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed('I'))
	{
		m_entity2->transform.position.y += 10.f * deltaSeconds;
	}

	if (g_inputSystem->WasKeyJustPressed(InputSystem::KEYBOARD_F1))
	{
		Mouse& mouse = InputSystem::GetMouse();
		mouse.ShowMouseCursor(!mouse.IsCursorShown());
		mouse.LockCursorToClient(!mouse.IsCursorLocked());
		mouse.SetCursorMode(mouse.GetCursorMode() == CURSORMODE_RELATIVE ? CURSORMODE_ABSOLUTE : CURSORMODE_RELATIVE);
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
}


//-------------------------------------------------------------------------------------------------
void Game::Update()
{
	const float deltaSeconds = m_gameClock->GetDeltaSeconds();
	m_collisionSystem->PerformBroadPhase();

	m_entity1->transform.Rotate(Vector3(0.f, 90.f * deltaSeconds, 0.f));

	const ContactManifold3d* man = m_collisionSystem->GetManifoldForColliders(m_entity1->GetCollider(), m_entity2->GetCollider());
	if (man && man->GetBroadphaseResult().m_collisionFound)
	{
		m_entity2->transform.position += man->GetBroadphaseResult().m_direction * man->GetBroadphaseResult().m_penetration;
	}
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

	Vector3 rot = m_entity1->transform.GetWorldRotationDegrees();
	Quaternion quat = Quaternion::FromEuler(rot);

	Renderable rend;
	rend.SetRenderableMatrix(m_entity1->transform.GetLocalToWorldMatrix());
	rend.AddDraw(m_mesh, m_material);
	g_renderContext->DrawRenderable(rend);
	
	rend.SetRenderableMatrix(m_entity2->transform.GetLocalToWorldMatrix());
	g_renderContext->DrawRenderable(rend);

	m_entity1->GetCollider()->DebugRender(m_material);
	m_entity2->GetCollider()->DebugRender(m_material);

	const ContactManifold3d* man = m_collisionSystem->GetManifoldForColliders(m_entity1->GetCollider(), m_entity2->GetCollider());
	if (man && man->GetBroadphaseResult().m_collisionFound)
	{
		g_renderContext->DrawLine3D(m_entity2->transform.position, m_entity2->transform.position + man->GetBroadphaseResult().m_direction * man->GetBroadphaseResult().m_penetration, m_material, Rgba::RED);
	}

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
}


//-------------------------------------------------------------------------------------------------
void Game::SetupRendering()
{
	// Cameras
	m_gameCamera = new Camera();
	m_gameCamera->SetProjectionPerspective(90.f, 0.1f, 100.f);
	m_gameCamera->LookAt(Vector3(0.f, 10.f, -10.f), Vector3(0.f, 0.f, 0.f));
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

	m_collisionSystem->AddEntity(m_entity1, colliderBounds);
	m_collisionSystem->AddEntity(m_entity2, colliderBounds);

	m_entity1->transform.position = Vector3(-20.f, 0.f, 0.f);
	m_entity1->transform.scale = Vector3(1.f);
	m_entity2->transform.position = Vector3(20.f, 0.f, 0.f);
	m_entity2->transform.SetRotation(Vector3(0.f, 45.f, 0.f));
}
