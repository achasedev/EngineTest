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
#include "Engine/UI/Panel.h"
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

void AFunction(const R<Texture2D>& hello)
{

	UNUSED(hello);
	int x = 0;
	x = 5;
}


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
	SAFE_DELETE_POINTER(m_obj2);
	SAFE_DELETE_POINTER(m_obj1);

	SAFE_DELETE_POINTER(m_material);
	SAFE_DELETE_POINTER(m_shader);
	SAFE_DELETE_POINTER(m_texture);
	SAFE_DELETE_POINTER(m_image);
	SAFE_DELETE_POINTER(m_uiCamera);
	SAFE_DELETE_POINTER(m_gameCamera);

	SAFE_DELETE_POINTER(m_physicsScene);
	SAFE_DELETE_POINTER(m_gameClock);
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

	if (g_inputSystem->WasKeyJustPressed('I'))
	{
		g_renderContext->SaveTextureToImage(g_renderContext->GetDefaultRenderTarget(), "Data/Screenshots/Font.png");
	}

	// Translate first object
	Vector2 polyTranslation = Vector2::ZERO;
	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_UP_ARROW)) { polyTranslation.y += 1.f; }		// Forward
	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_DOWN_ARROW)) { polyTranslation.y -= 1.f; }	// Back
	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_LEFT_ARROW)) { polyTranslation.x -= 1.f; }	// Left
	if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_RIGHT_ARROW)) { polyTranslation.x += 1.f; }	// Right

	m_obj1->m_transform.position += Vector3(polyTranslation * 200.f * deltaSeconds, 0.f);

	// Do the physics step
	m_physicsScene->FrameStep(deltaSeconds);
}


//-------------------------------------------------------------------------------------------------
void Game::Render()
{
	g_renderContext->BeginCamera(m_gameCamera);
	g_renderContext->ClearScreen(Rgba::BLACK);
	g_renderContext->ClearDepth();

	g_renderContext->BeginCamera(m_uiCamera);
	
	Polygon2D firstPoly, secondPoly;
	m_obj1->GetRigidBody2D()->GetWorldShape(firstPoly);
	m_obj2->GetRigidBody2D()->GetWorldShape(secondPoly);

	g_renderContext->DrawPolygon2D(firstPoly, m_material);
	g_renderContext->DrawPolygon2D(secondPoly, m_material);	
	
	g_renderContext->DrawPoint2D(m_obj1->GetRigidBody2D()->GetCenterOfMassWs(), 10.f, m_material);
	g_renderContext->DrawPoint2D(m_obj2->GetRigidBody2D()->GetCenterOfMassWs(), 10.f, m_material);

	Arbiter2D arb;
	bool arbFound = m_physicsScene->GetThatArbiter(&arb);
	if (arbFound)
	{
		const Contact2D* contacts = arb.GetContacts();
		uint32 numContacts = arb.GetNumContacts();
	
		ASSERT_OR_DIE(numContacts > 0, "Um hello?");
	
		for (uint32 i = 0; i < numContacts; ++i)
		{
			const Contact2D& currContact = contacts[i];
	
			g_renderContext->DrawPoint2D(currContact.m_position, 10.f, m_material, Rgba::RED);
	
			// Reference edge
			g_renderContext->DrawLine2D(currContact.m_referenceEdge.m_vertex1, currContact.m_referenceEdge.m_vertex2, m_material, Rgba::RED);
	
			// Contact normal/depth
			g_renderContext->DrawLine2D(currContact.m_position, currContact.m_position + (currContact.m_normal * currContact.m_separation), m_material, Rgba::CYAN);
	
			// Radius (center of masses to contact)
			g_renderContext->DrawLine2D(currContact.m_position - currContact.m_r1, currContact.m_position, m_material, Rgba::MAGENTA);
			g_renderContext->DrawLine2D(currContact.m_position - currContact.m_r2, currContact.m_position, m_material, Rgba::YELLOW);
		}
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
	m_physicsScene = new PhysicsScene2D();
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
	m_uiCamera->SetProjectionOrthographic(1000.f, g_window->GetClientAspect());

	// Shader
	m_shader = new Shader();
	m_shader->CreateFromFile("Data/Shader/test.shader");
	m_shader->SetBlend(BLEND_PRESET_ALPHA);
	m_shader->SetFillMode(FILL_MODE_WIREFRAME);

	// Texture
	m_image = new Image(IntVector2(2));

	m_texture = new Texture2D();
	m_texture->CreateFromImage(*m_image);
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
	Polygon2D* poly1 = new Polygon2D();
	poly1->AddVertex(Vector2(-100.f, -100.f));
	poly1->AddVertex(Vector2(-100.f, 100.f));
	poly1->AddVertex(Vector2(100.f, 100.f));
	poly1->AddVertex(Vector2(100.f, -100.f));

	Polygon2D* poly2 = new Polygon2D();
	poly2->AddVertex(Vector2(-300.f, -50.f));
	poly2->AddVertex(Vector2(-300.f, 50.f));
	poly2->AddVertex(Vector2(300.f, 50.f));
	poly2->AddVertex(Vector2(300.f, -50.f));


	m_obj1 = new GameObject();
	m_obj2 = new GameObject();

	m_obj1->m_transform.position = Vector3(g_window->GetClientBounds().GetCenter() + Vector2(0.f, 100.f), 0.f);
	m_obj1->m_transform.SetRotation(Vector3(0.f, 0.f, 30.f));
	m_obj2->m_transform.position = Vector3(g_window->GetClientBounds().GetCenter() - Vector2(0.f, 100.f), 0.f);

	m_obj1->SetShape(poly1);
	m_obj2->SetShape(poly2);

	m_physicsScene->AddGameObject(m_obj1);
	m_physicsScene->AddGameObject(m_obj2);

	m_obj1->GetRigidBody2D()->SetMassProperties(1.f);
	m_obj2->GetRigidBody2D()->SetAffectedByGravity(false);
}
