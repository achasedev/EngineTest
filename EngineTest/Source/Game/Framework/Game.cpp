///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 18th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Framework/EngineCommon.h"
#include "Game/Framework/Game.h"
#include "Engine/IO/Image.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Render/Camera/Camera.h"
#include "Engine/Render/Core/Renderable.h"
#include "Engine/Render/Core/RenderContext.h"
#include "Engine/Render/Material.h"
#include "Engine/Render/Mesh/Mesh.h"
#include "Engine/Render/Mesh/MeshBuilder.h"
#include "Engine/Render/Shader.h"
#include "Engine/Render/Texture/Texture2D.h"
#include "Engine/Render/Texture/TextureView2D.h"

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
	m_gameCamera = new Camera();
	m_gameCamera->SetProjectionPerspective(90.f, 0.1f, 100.f);
	m_gameCamera->LookAt(Vector3(0.f, 2.f, -2.f), Vector3(0.f, 0.f, 0.f));
	Vector3 forward = m_gameCamera->GetForwardVector();
	m_shader = new Shader();
	m_shader->CreateFromFile("Data/Shader/test.shader");

	m_mesh = new Mesh();

	// Meshbuild a quad
	MeshBuilder mb;
	mb.BeginBuilding(true);
	mb.PushCube(Vector3(0.f, 0.f, 0.f), Vector3::ONES);
	mb.FinishBuilding();

	mb.UpdateMesh<Vertex3D_PCU>(*m_mesh);
	mb.Clear();

	m_image = new Image();
	m_image->LoadFromFile("Data/Image/test.png");

	m_texture = new Texture2D();
	m_texture->CreateFromImage(*m_image);

	m_textureView = m_texture->CreateTextureView2D();

	m_material = new Material();
	m_material->SetShader(m_shader);
	m_material->SetAlbedoTextureView(m_textureView);

	m_parentRenderable = new Renderable();
	m_parentRenderable->AddDraw(m_mesh, m_material);

	m_childRenderable = new Renderable();
	m_childRenderable->AddDraw(m_mesh, m_material);

	m_parentTransform.SetRotation(Vector3(0.f, 30.f, 0.f));
	m_childTransform.SetPosition(Vector3(1.0f, 0.f, 0.0f));
	m_childTransform.scale = Vector3(0.5f);
	m_childTransform.SetParentTransform(&m_parentTransform);
	m_childTransform.SetRotation(Vector3(0.f, 45.f, 0.f));

	Mouse& mouse = InputSystem::GetMouse();
	mouse.ShowMouseCursor(false);
	mouse.LockCursorToClient(true);
	mouse.SetCursorMode(CURSORMODE_RELATIVE);
}


//-------------------------------------------------------------------------------------------------
Game::~Game()
{
	SAFE_DELETE_POINTER(m_textureView);
	SAFE_DELETE_POINTER(m_texture);
	SAFE_DELETE_POINTER(m_image);
	SAFE_DELETE_POINTER(m_mesh);
	SAFE_DELETE_POINTER(m_shader);
	SAFE_DELETE_POINTER(m_gameCamera);
}

//-------------------------------------------------------------------------------------------------
void Game::Update()
{
	static float test = 0.f;
	test += 0.005f;

	m_parentTransform.SetRotation(Vector3(0.f, 0.5f * -test, 0.f));
	m_childTransform.position = Vector3(CosDegrees(test), 0.f, SinDegrees(test));
	m_childTransform.SetRotation(Vector3(0.f, -test, 0.f));
	m_parentRenderable->SetRenderableMatrix(m_parentTransform.GetLocalToWorldMatrix());
	m_childRenderable->SetRenderableMatrix(m_childTransform.GetLocalToWorldMatrix());

	// Update da camera
	InputSystem* input = InputSystem::GetInstance();

	// Translating the camera
	Vector3 translationOffset = Vector3::ZERO;
	if (input->IsKeyPressed('W')) { translationOffset.z += 1.f; }		// Forward
	if (input->IsKeyPressed('S')) { translationOffset.z -= 1.f; }		// Left
	if (input->IsKeyPressed('A')) { translationOffset.x -= 1.f; }		// Back
	if (input->IsKeyPressed('D')) { translationOffset.x += 1.f; }		// Right
	if (input->IsKeyPressed(InputSystem::KEYBOARD_SPACEBAR)) { translationOffset.y += 1.f; }		// Up
	if (input->IsKeyPressed('X')) { translationOffset.y -= 1.f; }		// Down

	if (input->IsKeyPressed(InputSystem::KEYBOARD_SHIFT))
	{
		translationOffset *= 50.f;
	}

	const float deltaTime = (1.f / 200.f); // 200 fps?
	translationOffset *= (1.f * deltaTime);

	if (translationOffset.z > 0.f)
	{
		int x = 0;
		x = 5;
	}

	m_gameCamera->Translate(translationOffset);

	// Rotating the camera
	Mouse& mouse = InputSystem::GetMouse();
	IntVector2 mouseDelta = mouse.GetMouseDelta();

	Vector2 rotationOffset = Vector2((float)mouseDelta.y, (float)mouseDelta.x) * 0.12f;
	Vector3 rotation = Vector3(rotationOffset.x * 60.f * deltaTime, rotationOffset.y * 60.f * deltaTime, 0.f);

	m_gameCamera->Rotate(rotation);
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	RenderContext* renderContext = RenderContext::GetInstance();

	renderContext->BeginCamera(m_gameCamera);
	renderContext->ClearCurrentColorTargetView(Rgba::BLACK);
	renderContext->ClearCurrentDepthStencilTargetView();

	renderContext->DrawRenderable(*m_parentRenderable);
	renderContext->DrawRenderable(*m_childRenderable);

	renderContext->EndCamera();
}