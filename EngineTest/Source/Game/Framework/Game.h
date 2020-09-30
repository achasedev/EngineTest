///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 14th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------
#pragma once

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Framework/Rgba.h"
#include "Engine/Math/Transform.h"
#include "Engine/Physics/RigidBody2D.h"
#include <vector>

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#define NUM_PLINKOS 50

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
class Camera;
class Canvas;
class Clock;
class FontAtlas;
class Image;
class Material;
class Mesh;
class Panel;
class Polygon2D;
class Renderable;
class Shader;
class ShaderResourceView;
class Texture2D;
class UIText;
class PhysicsScene2D;

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
class Game
{
public:
	//-----Public Methods-----


private:
	//-----Private Methods-----

	friend class App;

	Game();
	~Game();
	Game(const Game& copy) = delete;

	void Update();
	void Render();


private:
	//-----Private Methods-----

	void SetupFramework();
	void SetupRendering();
	void SetupObjects();


private:
	//-----Private Data-----

	// Rendering
	Camera*				m_gameCamera = nullptr;
	Camera*				m_uiCamera = nullptr;
	Shader*				m_shader = nullptr;
	Image*				m_image = nullptr;
	Texture2D*			m_texture = nullptr;
	ShaderResourceView* m_textureView = nullptr;
	Material*			m_material = nullptr;

	// Framework
	Clock* m_gameClock = nullptr;

	// Objects
	PhysicsScene2D* m_physicsScene = nullptr;
	//GameObject* m_obj1 = nullptr;
	//GameObject* m_obj2 = nullptr;

	//GameObject* m_objects[10];


	// Plinko!

	Polygon2D*	m_floorPoly = nullptr;
	Polygon2D*	m_wallPoly = nullptr;
	Polygon2D*	m_trianglePoly = nullptr;
	Polygon2D*	m_circlePoly = nullptr;

	GameObject* m_floorObj = nullptr;
	GameObject* m_leftWallObj = nullptr;
	GameObject* m_rightWallObj = nullptr;

	GameObject* m_triangles[50];
	GameObject* m_plinkos[NUM_PLINKOS];
};


///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
