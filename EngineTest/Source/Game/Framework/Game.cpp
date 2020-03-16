///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 18th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Framework/Game.h"
#include "Engine/DirectX/Camera.h"
#include "Engine/DirectX/Material.h"
#include "Engine/DirectX/Mesh.h"
#include "Engine/DirectX/MeshBuilder.h"
#include "Engine/DirectX/Renderable.h"
#include "Engine/DirectX/RenderContext.h"
#include "Engine/DirectX/Shader.h"
#include "Engine/Framework/EngineCommon.h"
#include "Engine/IO/Image.h"
#include "Engine/DirectX/Texture2D.h"
#include "Engine/DirectX/TextureView2D.h"

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
	
	m_shader = new Shader();
	m_shader->CreateFromFile("Data/Shader/test.shader");
	
	m_mesh = new Mesh();

	// Meshbuild a square
	MeshBuilder mb;
	mb.BeginBuilding(true);

	mb.SetColor(Rgba::RED);
	mb.SetUV(Vector2(0.f, 0.f));
	uint32 firstIndex = mb.PushVertex(Vector3(-0.5f, -0.5f, 0.0f));

	mb.SetColor(Rgba::GREEN);
	mb.SetUV(Vector2(0.f, 1.f));
	mb.PushVertex(Vector3(-0.5f, 0.5f, 0.0f));

	mb.SetColor(Rgba::BLUE);
	mb.SetUV(Vector2(1.f, 1.f));
	mb.PushVertex(Vector3(0.5f, 0.5f, 0.0f));

	mb.SetColor(Rgba(1.0f, 1.0f, 0.f, 1.0f));
	mb.SetUV(Vector2(1.f, 0.f));
	mb.PushVertex(Vector3(0.5f, -0.5f, 0.0f));

	mb.PushIndex(firstIndex);
	mb.PushIndex(firstIndex + 1);
	mb.PushIndex(firstIndex + 2);

	mb.PushIndex(firstIndex);
	mb.PushIndex(firstIndex + 2);
	mb.PushIndex(firstIndex + 3);

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

	m_renderable = new Renderable();
	m_renderable->AddDraw(m_mesh, m_material);
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
	
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	RenderContext* renderContext = RenderContext::GetInstance();

	renderContext->BeginCamera(m_gameCamera);

	renderContext->DrawRenderable(*m_renderable);

	renderContext->EndCamera();
}