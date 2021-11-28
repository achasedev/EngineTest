///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: December 14th, 2019
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------
#pragma once

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Math/Transform.h"
#include "Engine/Collision/BoundingVolumeHierarchy/BoundingVolume.h"
#include "Engine/Collision/CollisionScene.h"
#include <vector>

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
class Camera;
class Clock;
class Entity;
class ForwardRenderer;
class Light;
class Particle;
class ParticleWorld;
class PhysicsScene;
class Player;
class RenderScene;
class RigidBody;

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

	CollisionScene<BoundingVolumeSphere>* GetCollisionScene() const { return m_collisionScene; }


private:
	//-----Private Methods-----

	friend class App;

	Game();
	~Game();
	Game(const Game& copy) = delete;

	void ProcessInput();
	void Update();
	void Render();


private:
	//-----Private Methods-----

	void SetupFramework();
	void SetupRendering();
	void SpawnEntities();

	void PreUpdate(float deltaSeconds);
	void PhysicsUpdate(float deltaSeconds);
	void PostUpdate(float deltaSeconds);
	void UpdateRenderables();

	// Physics helpers
	void SpawnCapsule(float cylinderHeight, float radius, float inverseMass, const Vector3& position, const Vector3& rotationDegrees = Vector3::ZERO, const Vector3& velocity = Vector3::ZERO, const Vector3& angularVelocityDegrees = Vector3::ZERO, bool hasGravity = true);
	void SpawnBox(const Vector3& extents, float inverseMass, const Vector3& position, const Vector3& rotationDegrees = Vector3::ZERO, const Vector3& velocity = Vector3::ZERO, const Vector3& angularVelocityDegrees = Vector3::ZERO, bool hasGravity = true);
	void SpawnSphere(float radius, float inverseMass, const Vector3& position, const Vector3& rotationDegrees = Vector3::ZERO, const Vector3& velocity = Vector3::ZERO, const Vector3& angularVelocityDegrees = Vector3::ZERO, bool hasGravity = true);
	void SpawnCylinder(float height, float radius, float inverseMass, const Vector3& position, const Vector3& rotationDegrees = Vector3::ZERO, const Vector3& velocity = Vector3::ZERO, const Vector3& angularVelocityDegrees = Vector3::ZERO, bool hasGravity = true);

	void SpawnGround();
	void SpawnPolygon(float inverseMass, const Vector3& position, const Vector3& rotationDegrees = Vector3::ZERO, const Vector3& velocity = Vector3::ZERO, const Vector3& angularVelocityDegrees = Vector3::ZERO, bool hasGravity = true);
	void SpawnPolygon2(float inverseMass, const Vector3& position, const Vector3& rotationDegrees = Vector3::ZERO, const Vector3& velocity = Vector3::ZERO, const Vector3& angularVelocityDegrees = Vector3::ZERO, bool hasGravity = true);

	void MakeScalarField();


private:
	//-----Private Data-----

	// Rendering
	Camera*										m_gameCamera = nullptr;
	Camera*										m_uiCamera = nullptr;
	ForwardRenderer*							m_renderer = nullptr;
	RenderScene*								m_renderScene = nullptr;
	bool										m_drawColliders = false;
	Light*										m_coneLight = nullptr;

	// Framework
	Clock*										m_gameClock = nullptr;

	// Physics/collision
	bool										m_pausePhysics = true;
	PhysicsScene*								m_physicsScene = nullptr;
	CollisionScene<BoundingVolumeSphere>*		m_collisionScene = nullptr;
	int											m_spawnType = 0;
	Polyhedron									m_poly[4];
	Mesh*										m_polyMesh[4];

	// Entities
	Player*										m_player = nullptr;
	Entity*										m_ground = nullptr;
	Entity*										m_light = nullptr;
	std::vector<Entity*>						m_entities;

};


///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
