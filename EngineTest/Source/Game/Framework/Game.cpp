///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 18th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Framework/DevConsole.h"
#include "Game/Framework/Game.h"
#include "Game/Framework/GameJobs.h"
#include "Engine/Framework/EngineCommon.h"
#include "Engine/Framework/GameObject.h"
#include "Engine/Framework/Window.h"
#include "Engine/IO/Image.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Job/JobSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/OBB2.h"
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
	SAFE_DELETE(m_secondObj);
	SAFE_DELETE(m_firstObj);

	SAFE_DELETE(m_cubePoly);

	SAFE_DELETE(m_material);
	SAFE_DELETE(m_shader);
	SAFE_DELETE(m_texture);
	SAFE_DELETE(m_image);
	SAFE_DELETE(m_uiCamera);
	SAFE_DELETE(m_gameCamera);

	SAFE_DELETE(m_physicsScene);
	SAFE_DELETE(m_timer);
	SAFE_DELETE(m_gameClock);
}


//-------------------------------------------------------------------------------------------------
void Game::ProcessInput()
{
	const float deltaSeconds = m_gameClock->GetDeltaSeconds();

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_LEFT_ARROW))
	{
		m_secondObj->m_transform.position.x += -10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_RIGHT_ARROW))
	{
		m_secondObj->m_transform.position.x += 10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_DOWN_ARROW))
	{
		m_secondObj->m_transform.position.z += -10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_UP_ARROW))
	{
		m_secondObj->m_transform.position.z += 10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed('K'))
	{
		m_secondObj->m_transform.position.y += -10.f * deltaSeconds;
	}

	if (g_inputSystem->IsKeyPressed('I'))
	{
		m_secondObj->m_transform.position.y += 10.f * deltaSeconds;
	}
}


//-------------------------------------------------------------------------------------------------
void Game::Update()
{
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
	if (g_inputSystem->IsKeyPressed('W'))								{ translationOffset.z += 1.f; }		// Forward
	if (g_inputSystem->IsKeyPressed('S'))								{ translationOffset.z -= 1.f; }		// Left
	if (g_inputSystem->IsKeyPressed('A'))								{ translationOffset.x -= 1.f; }		// Back
	if (g_inputSystem->IsKeyPressed('D'))								{ translationOffset.x += 1.f; }		// Right
	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SPACEBAR))	{ translationOffset.y += 1.f; }		// Up
	if (g_inputSystem->IsKeyPressed('X'))								{ translationOffset.y -= 1.f; }		// Down

	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SHIFT))
	{
		translationOffset *= 50.f;
	}

	const float deltaSeconds = m_gameClock->GetDeltaSeconds();
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

	m_physicsScene->FrameStep(deltaSeconds);

	Arbiter3D* arb = m_physicsScene->GetArbiter3DForBodies(m_firstObj->GetRigidBody3D(), m_secondObj->GetRigidBody3D());
	
	if (arb != nullptr)
	{
		CollisionSeparation3D sep = arb->GetSeparation();
		if (sep.m_collisionFound)
		{
			m_secondObj->m_transform.position += sep.m_dirFromFirst * sep.m_separation;
		}
	}
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

	Arbiter3D* arb = m_physicsScene->GetArbiter3DForBodies(m_firstObj->GetRigidBody3D(), m_secondObj->GetRigidBody3D());
	if (arb != nullptr && arb->GetSeparation().m_collisionFound)
	{
		m_firstObj->GetShape3D()->DebugRender(&m_firstObj->m_transform, m_material, Rgba::RED);
		m_secondObj->GetShape3D()->DebugRender(&m_secondObj->m_transform, m_material, Rgba::RED);
	}
	else
	{
		m_firstObj->GetShape3D()->DebugRender(&m_firstObj->m_transform, m_material, Rgba::BLUE);
		m_secondObj->GetShape3D()->DebugRender(&m_secondObj->m_transform, m_material, Rgba::YELLOW);
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

	m_physicsScene = new PhysicsScene3D();
	m_physicsScene->SetGravity(Vector3(0.f, -9.8f, 0.f));
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
}


//-------------------------------------------------------------------------------------------------
void Game::SetupObjects()
{
	// *Local* space defined
	m_cubePoly = new Polygon3D();
	float cubeSize = 5.f;

	m_cubePoly->PushVertex(Vector3(-cubeSize, -cubeSize, -cubeSize));
	m_cubePoly->PushVertex(Vector3(-cubeSize, cubeSize, -cubeSize));
	m_cubePoly->PushVertex(Vector3(cubeSize, cubeSize, -cubeSize));
	m_cubePoly->PushVertex(Vector3(cubeSize, -cubeSize, -cubeSize));
	m_cubePoly->PushVertex(Vector3(cubeSize, -cubeSize, cubeSize));
	m_cubePoly->PushVertex(Vector3(cubeSize, cubeSize, cubeSize));
	m_cubePoly->PushVertex(Vector3(-cubeSize, cubeSize, cubeSize));
	m_cubePoly->PushVertex(Vector3(-cubeSize, -cubeSize, cubeSize));
	
	// Left/Right
	m_cubePoly->PushIndex(7);
	m_cubePoly->PushIndex(6);
	m_cubePoly->PushIndex(1);
	m_cubePoly->PushIndex(0);

	m_cubePoly->PushIndex(3);
	m_cubePoly->PushIndex(2);
	m_cubePoly->PushIndex(5);
	m_cubePoly->PushIndex(4);

	//m_cubePoly->PushIndicesForTriangle(7, 6, 1);
	//m_cubePoly->PushIndicesForTriangle(7, 1, 0);
	//m_cubePoly->PushIndicesForTriangle(3, 2, 5);
	//m_cubePoly->PushIndicesForTriangle(3, 5, 4);

	// Bottom/Top
	m_cubePoly->PushIndex(7);
	m_cubePoly->PushIndex(0);
	m_cubePoly->PushIndex(3);
	m_cubePoly->PushIndex(4);

	m_cubePoly->PushIndex(1);
	m_cubePoly->PushIndex(6);
	m_cubePoly->PushIndex(5);
	m_cubePoly->PushIndex(2);

	//m_cubePoly->PushIndicesForTriangle(7, 0, 3);
	//m_cubePoly->PushIndicesForTriangle(7, 3, 4);
	//m_cubePoly->PushIndicesForTriangle(1, 6, 5);
	//m_cubePoly->PushIndicesForTriangle(1, 5, 2);

	// Back/Front
	m_cubePoly->PushIndex(0);
	m_cubePoly->PushIndex(1);
	m_cubePoly->PushIndex(2);
	m_cubePoly->PushIndex(3);

	m_cubePoly->PushIndex(4);
	m_cubePoly->PushIndex(5);
	m_cubePoly->PushIndex(6);
	m_cubePoly->PushIndex(7);

	//m_cubePoly->PushIndicesForTriangle(0, 1, 2);
	//m_cubePoly->PushIndicesForTriangle(0, 2, 3);
	//m_cubePoly->PushIndicesForTriangle(4, 5, 6);
	//m_cubePoly->PushIndicesForTriangle(4, 6, 7);

	// 6 faces, each with 4 vertices
	m_cubePoly->PushFaceIndexCount(4);
	m_cubePoly->PushFaceIndexCount(4);
	m_cubePoly->PushFaceIndexCount(4);
	m_cubePoly->PushFaceIndexCount(4);
	m_cubePoly->PushFaceIndexCount(4);
	m_cubePoly->PushFaceIndexCount(4);

	Polygon3D* trigPoly = new Polygon3D();
	trigPoly->PushVertex(Vector3(-1.0f, -1.0f, -1.0f));
	trigPoly->PushVertex(Vector3(1.0f, -1.0f, -1.0f));
	trigPoly->PushVertex(Vector3(0.f, -1.0f, 1.0f));
	trigPoly->PushVertex(Vector3(0.f, 1.0f, 0.f));

	trigPoly->PushIndex(0);
	trigPoly->PushIndex(1);
	trigPoly->PushIndex(2);

	trigPoly->PushIndex(0);
	trigPoly->PushIndex(2);
	trigPoly->PushIndex(3);

	trigPoly->PushIndex(1);
	trigPoly->PushIndex(3);
	trigPoly->PushIndex(2);

	trigPoly->PushIndex(0);
	trigPoly->PushIndex(3);
	trigPoly->PushIndex(1);

	trigPoly->PushFaceIndexCount(3);
	trigPoly->PushFaceIndexCount(3);
	trigPoly->PushFaceIndexCount(3);
	trigPoly->PushFaceIndexCount(3);

	m_firstObj = new GameObject();
	m_secondObj = new GameObject();

	m_firstObj->SetShape3D(m_cubePoly);
	m_secondObj->SetShape3D(trigPoly);

	m_firstObj->m_transform.position = Vector3(-20.f, 0.f, 0.f);
	m_secondObj->m_transform.position = Vector3(20.f, 0.f, 0.f);

	m_physicsScene->AddGameObject(m_firstObj);
	m_physicsScene->AddGameObject(m_secondObj);
}
