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
#include "Engine/Framework/EngineCommon.h"
#include "Engine/Framework/GameObject.h"
#include "Engine/Framework/Window.h"
#include "Engine/IO/Image.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Job/JobSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/OBB2.h"
#include "Engine/Math/Polygon2D.h"
#include "Engine/Physics/Physics2D.h"
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
#include "Engine/Time/Time.h"
#include "Engine/Utility/NamedProperties.h"
#include "Engine/Utility/SmartPointer.h"
#include "Engine/Utility/StringID.h"
#include "Engine/UI/Canvas.h"
#include "Engine/UI/UIPanel.h"
#include "Engine/UI/UIText.h"
#include "Engine/Voxel/QEFLoader.h"
#include "Engine/Physics/Arbiter2D.h"

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
	for (int i = 0; i < 50; ++i)
	{
		SAFE_DELETE(m_triangles[i]);
	}

	for (int i = 0; i < NUM_PLINKOS; ++i)
	{
		SAFE_DELETE(m_plinkos[i]);
	}

	SAFE_DELETE(m_floorObj);
	SAFE_DELETE(m_leftWallObj);
	SAFE_DELETE(m_rightWallObj);

	SAFE_DELETE(m_floorPoly);
	SAFE_DELETE(m_wallPoly);
	SAFE_DELETE(m_trianglePoly);
	SAFE_DELETE(m_circlePoly);

	SAFE_DELETE(m_material);
	SAFE_DELETE(m_shader);
	SAFE_DELETE(m_texture);
	SAFE_DELETE(m_image);
	SAFE_DELETE(m_uiCamera);
	SAFE_DELETE(m_gameCamera);

	SAFE_DELETE(m_physicsScene);
	SAFE_DELETE(m_gameClock);
}


//-------------------------------------------------------------------------------------------------
void Game::ProcessInput()
{
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
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

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
	m_physicsScene = new PhysicsScene2D();
	m_physicsScene->SetGravity(Vector2(0.f, -1000.f));
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
	m_floorPoly = new Polygon2D();
	m_floorPoly->AddVertex(Vector2(-1200.f, -50.f));
	m_floorPoly->AddVertex(Vector2(-1200.f, 0.f));
	m_floorPoly->AddVertex(Vector2(1200.f, 0.f));
	m_floorPoly->AddVertex(Vector2(1200.f, -50.f));

	m_wallPoly = new Polygon2D();
	m_wallPoly->AddVertex(Vector2(-25.f, -400.f));
	m_wallPoly->AddVertex(Vector2(-25.f, 400.f));
	m_wallPoly->AddVertex(Vector2(25.f, 400.f));
	m_wallPoly->AddVertex(Vector2(25.f, -400.f));

	m_trianglePoly = new Polygon2D();
	m_trianglePoly->AddVertex(Vector2(-25.f, -10.f));
	m_trianglePoly->AddVertex(Vector2(0.f, 10.f));
	m_trianglePoly->AddVertex(Vector2(25.f, -10.f));

	m_circlePoly = new Polygon2D();
	for (int i = 19; i >= 0; --i)
	{
		float radius = 20.f;
		float angle = (float)i * (360.f / 20.f);
		m_circlePoly->AddVertex(radius * Vector2(CosDegrees(angle), SinDegrees(angle)));
	}

	m_floorObj = new GameObject();
	m_leftWallObj = new GameObject();
	m_rightWallObj = new GameObject();

	m_floorObj->SetShape(m_floorPoly);
	m_leftWallObj->SetShape(m_wallPoly);
	m_rightWallObj->SetShape(m_wallPoly);

	m_floorObj->m_transform.position = Vector3(m_uiCamera->GetOrthoBounds().GetCenter().x, 50.f, 0.f);
	m_leftWallObj->m_transform.position = Vector3(25.f, 450.f, 0.f);
	m_rightWallObj->m_transform.position = Vector3(m_uiCamera->GetOrthoBounds().maxs.x - 25.f, 450.f, 0.f);

	m_physicsScene->AddGameObject(m_floorObj);
	m_physicsScene->AddGameObject(m_leftWallObj);
	m_physicsScene->AddGameObject(m_rightWallObj);

	m_floorObj->GetRigidBody2D()->SetAffectedByGravity(false);
	m_leftWallObj->GetRigidBody2D()->SetAffectedByGravity(false);
	m_rightWallObj->GetRigidBody2D()->SetAffectedByGravity(false);

	for (int i = 0; i < 50; ++i)
	{
		m_triangles[i] = new GameObject();
		m_triangles[i]->SetShape(m_trianglePoly);

		int row = i % 10;
		int col = i / 10;

		if (col % 2 == 0)
		{
			m_triangles[i]->m_transform.position = Vector3(200.f * (float)row + 150.f, 150.f * (float)col + 150.f, 0.f);
		}
		else
		{
			m_triangles[i]->m_transform.position = Vector3(200.f * (float)row + 275.f, 150.f * (float)col + 150.f, 0.f);
		}

		m_physicsScene->AddGameObject(m_triangles[i]);
		m_triangles[i]->GetRigidBody2D()->SetAffectedByGravity(false);
	}

	for (int i = 0; i < NUM_PLINKOS; ++i)
	{
		m_plinkos[i] = new GameObject();
		m_plinkos[i]->SetShape(m_circlePoly);

		int row = i % 10;
		int col = i / 10;

		if (col % 2 == 0)
		{
			m_plinkos[i]->m_transform.position = Vector3(200.f * (float)row + 150.f, 40.f * (float)col + 800.f, 0.f);
		}
		else
		{
			m_plinkos[i]->m_transform.position = Vector3(200.f * (float)row + 250.f, 40.f * (float)col + 800.f, 0.f);
		}

		m_physicsScene->AddGameObject(m_plinkos[i]);
		m_plinkos[i]->GetRigidBody2D()->SetMassProperties(1.0f);
	}
}
