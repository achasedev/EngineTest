///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 18th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Framework/Game.h"
#include "Engine/Render/Camera/Camera.h"
#include "Engine/Render/Material.h"
#include "Engine/Render/Mesh/Mesh.h"
#include "Engine/Render/Mesh/MeshBuilder.h"
#include "Engine/Render/Core/Renderable.h"
#include "Engine/Render/Core/RenderContext.h"
#include "Engine/Render/Shader.h"
#include "Engine/Framework/EngineCommon.h"
#include "Engine/IO/Image.h"
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
#include "Engine/Math/MathUtils.h"
void Game::Update()
{
	static float test = 0.f;
	test += 0.005f;

	m_parentTransform.SetRotation(Vector3(0.f, 0.5f * -test, 0.f));
	m_childTransform.position = Vector3(CosDegrees(test), 0.f, SinDegrees(test));
	m_childTransform.SetRotation(Vector3(0.f, -test, 0.f));
	m_parentRenderable->SetRenderableMatrix(m_parentTransform.GetLocalToWorldMatrix());
	m_childRenderable->SetRenderableMatrix(m_childTransform.GetLocalToWorldMatrix());
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