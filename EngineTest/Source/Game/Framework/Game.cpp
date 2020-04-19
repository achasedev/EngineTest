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

	m_uiCamera = new Camera();
	m_uiCamera->SetDepthStencilTargetView(nullptr, false);
	m_uiCamera->SetProjectionOrthographic(1080.f, 2.33f);

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

	Texture2D* testTexture = new Texture2D();
	testTexture->CreateFromImage(*m_image);

	m_textureView = m_texture->CreateOrGetShaderResourceView();

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

	//Mouse& mouse = InputSystem::GetMouse();
	//mouse.ShowMouseCursor(false);
	//mouse.LockCursorToClient(true);
	//mouse.SetCursorMode(CURSORMODE_RELATIVE);


	NamedProperties prop;

	prop.Set("Health", 5.0f);
	prop.Set("Age", 4);

	std::string name = "Name";
	std::string dimensions = "Dimensions";

	prop.Set(name, "Andrew");
	prop.Set(dimensions, Vector2(3.4f, 7.8f));

	prop.Set(SID("Health"), 7.0f);

	DebuggerPrintf("%s", prop.ToString().c_str());

	m_gameClock = new Clock(nullptr);

	R<Texture2D> sp = m_texture;

	if (sp == nullptr)
	{
		int x = 0;
		x = 5;
	}
	
	uint64 startHPC = GetPerformanceCounter();
	Vector3 myVectorTest = Vector3(5.f, 5.f, 4.f);

	for (int i = 0; i < 100000; ++i)
	{
		AFunction(sp);
	}
	g_renderContext->BindShaderResourceView(0, m_textureView);
	uint64 endHPC = GetPerformanceCounter();
	float milliseconds = (float)TimeSystem::PerformanceCountToSeconds(endHPC - startHPC) * 1000.f;
	DebuggerPrintf("\n\nTook %.3f milliseconds\n\n", milliseconds);

	m_canvas = new Canvas();
	m_canvas->Initialize(m_uiCamera, Vector2(1080.f * g_window->GetClientAspect(), 1080.f), SCREEN_MATCH_WIDTH_OR_HEIGHT);

	m_panel1 = new Panel(m_canvas);
	m_panel1->SetCanvas(m_canvas);
	m_panel1->m_transform.SetAnchors(AnchorPreset::BOTTOM_LEFT);
	m_panel1->m_transform.SetPivot(Vector2(0.f, 0.0f));
	m_panel1->m_transform.SetPosition(Vector2::ZERO);
	m_panel1->m_transform.SetDimensions(Vector2(512.f, 512.f));

	//panel->m_transform.SetHorizontalPadding(0.f, 300.f);
	//panel->m_transform.SetVerticalPadding(0.f, 300.f);
	m_canvas->AddChild(m_panel1);



	mb.Clear();
	mb.BeginBuilding(true);

	mb.PushQuad2D(AABB2::ZERO_TO_ONE);
	mb.FinishBuilding();
	Mesh* mesh = mb.CreateMesh<Vertex3D_PCU>();

	Renderable* panelRend = new Renderable();
	panelRend->AddDraw(mesh, m_material);
	m_panel1->SetRenderable(panelRend);

	Font* font = g_FontLoader->LoadFont("Data/Fonts/test.ttf", 0);
	FontAtlas* atlas = font->CreateOrGetAtlasForPixelHeight(4U);
	AABB2 glyphUVs = atlas->CreateOrGetUVsForGlyph('A');

	m_panel2 = new Panel(m_canvas);
	m_panel2->SetCanvas(m_canvas);
	m_panel2->m_transform.SetAnchors(AnchorPreset::TOP_LEFT);
	m_panel2->m_transform.SetPivot(Vector2(0.f, 1.0f));
	m_panel2->m_transform.SetPosition(Vector2::ZERO);
	m_panel2->m_transform.SetDimensions(Vector2(150.f * glyphUVs.GetAspect(), 150.f));
	Renderable* panel2Rend = new Renderable();

	mb.Clear();
	mb.BeginBuilding(true);

	mb.PushQuad2D(AABB2::ZERO_TO_ONE, glyphUVs);
	mb.FinishBuilding();
	Mesh* mesh2 = mb.CreateMesh<Vertex3D_PCU>();

	Material* testMat = new Material();
	testMat->SetShader(m_shader);
	testMat->SetAlbedoTextureView(atlas->GetTexture()->CreateOrGetShaderResourceView());
	panel2Rend->AddDraw(mesh2, testMat);
	m_panel2->SetRenderable(panel2Rend);
	m_canvas->AddChild(m_panel2);
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
	static float test = 0.f;
	test += 0.005f;

	m_parentTransform.SetRotation(Vector3(0.f, 0.5f * -test, 0.f));
	m_childTransform.position = Vector3(CosDegrees(test), 0.f, SinDegrees(test));
	m_childTransform.SetRotation(Vector3(0.f, -test, 0.f));
	m_parentRenderable->SetRenderableMatrix(m_parentTransform.GetLocalToWorldMatrix());
	m_childRenderable->SetRenderableMatrix(m_childTransform.GetLocalToWorldMatrix());

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

	const float deltaTime = (1.f / 200.f); // 200 fps?
	translationOffset *= (1.f * deltaTime);

	m_gameCamera->Translate(translationOffset);

	// Rotating the camera
	Mouse& mouse = InputSystem::GetMouse();
	IntVector2 mouseDelta = mouse.GetMouseDelta();

	Vector2 rotationOffset = Vector2((float)mouseDelta.y, (float)mouseDelta.x) * 0.12f;
	Vector3 rotation = Vector3(rotationOffset.x * 200.f * deltaTime, rotationOffset.y * 200.f * deltaTime, 0.f);

	m_gameCamera->Rotate(rotation);

	DebuggerPrintf("Frame: %.5f | Total: %.5f | FPS: %.2f\n", m_gameClock->GetDeltaSeconds(), m_gameClock->GetTotalSeconds(), 1.0f / m_gameClock->GetDeltaSeconds());

	m_panel1->m_transform.SetXPosition(500.f * (SinDegrees(m_gameClock->GetTotalSeconds() * 90.f) + 1.0f));
	//m_panel2->m_transform.SetYPosition(500.f * (SinDegrees(m_gameClock->GetTotalSeconds() * 90.f) + 1.0f));
	//m_panel2->m_transform.SetOrientation(m_gameClock->GetTotalSeconds() * 90.f);
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

	g_renderContext->DrawRenderable(*m_parentRenderable);
	g_renderContext->DrawRenderable(*m_childRenderable);

	g_renderContext->BeginCamera(m_uiCamera);
	m_canvas->Render();

	g_renderContext->EndCamera();
}