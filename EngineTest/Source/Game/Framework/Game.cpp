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
#include "Engine/Framework/Window.h"
#include "Engine/IO/Image.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Job/JobSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/OBB2.h"
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
#include "Engine/UI/Panel.h"
#include "Engine/UI/UIText.h"
#include "Engine/Voxel/QEFLoader.h"

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

void AFunction(const R<Texture2D>& hello)
{

	UNUSED(hello);
	int x = 0;
	x = 5;
}


//-------------------------------------------------------------------------------------------------
Game::Game()
{
	m_gameCamera = new Camera();
	m_gameCamera->SetProjectionPerspective(90.f, 0.1f, 100.f);
	m_gameCamera->LookAt(Vector3(10.f, 0.f, 0.f), Vector3(0.f, 0.f, 0.f));
	m_gameCamera->SetDepthTarget(g_renderContext->GetDefaultDepthStencilTarget(), false);

	m_shader = new Shader();
	m_shader->CreateFromFile("Data/Shader/test.shader");
	m_shader->SetBlend(BLEND_PRESET_ALPHA);
	m_shader->SetFillMode(FILL_MODE_WIREFRAME);

	m_mesh = new Mesh();

	// Meshbuild a quad
	MeshBuilder mb;
	mb.BeginBuilding(true);
	mb.PushCube(Vector3(0.f, 0.f, 0.f), Vector3::ONES);
	mb.FinishBuilding();

	mb.UpdateMesh<Vertex3D_PCU>(*m_mesh);
	mb.Clear();

	m_image = new Image(IntVector2(2));

	m_texture = new Texture2D();
	m_texture->CreateFromImage(*m_image);
	m_textureView = m_texture->CreateOrGetShaderResourceView();

	m_material = new Material();
	m_material->SetShader(m_shader);
	m_material->SetAlbedoTextureView(m_textureView);

	Mouse& mouse = InputSystem::GetMouse();
	mouse.ShowMouseCursor(false);
	mouse.LockCursorToClient(true);
	mouse.SetCursorMode(CURSORMODE_RELATIVE);

	m_gameClock = new Clock(nullptr);
	
	QEFLoader loader;
	loader.LoadFile("Data/Mesh/test.qef");
	Mesh* mesh = loader.CreateMesh();

	m_voxelRenderable = new Renderable();
	m_voxelRenderable->AddDraw(mesh, m_material);
}


//-------------------------------------------------------------------------------------------------
Game::~Game()
{
	SAFE_DELETE_POINTER(m_textureView);
	SAFE_DELETE_POINTER(m_image);
	SAFE_DELETE_POINTER(m_mesh);
	SAFE_DELETE_POINTER(m_shader);
	SAFE_DELETE_POINTER(m_gameCamera);
}

//-------------------------------------------------------------------------------------------------
void Game::Update()
{
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

	const float deltaTime = m_gameClock->GetDeltaSeconds();
	translationOffset *= deltaTime;

	m_gameCamera->Translate(translationOffset);

	// Rotating the camera
	Mouse& mouse = InputSystem::GetMouse();
	IntVector2 mouseDelta = mouse.GetMouseDelta();

	Vector2 rotationOffset = Vector2((float)mouseDelta.y, (float)mouseDelta.x);
	Vector3 deltaRotation = Vector3(rotationOffset.x * 90.f * deltaTime, rotationOffset.y * 90.f * deltaTime, 0.f);
	m_gameCamera->SetRotation(m_gameCamera->GetRotation() + deltaRotation);

	if (g_inputSystem->WasKeyJustPressed('I'))
	{
		g_renderContext->SaveTextureToImage(g_renderContext->GetDefaultRenderTarget(), "Data/Screenshots/Font.png");
	}
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

	g_renderContext->DrawRenderable(*m_voxelRenderable);

	g_renderContext->EndCamera();
}