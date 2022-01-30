///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: Jan 21st, 2022
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------
#pragma once

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Math/IntVector2.h"
#include "Engine/Render/Renderable.h"
#include <map>

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
class Chunk;
class RenderScene;

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
class World
{
public:
	//-----Public Methods-----

	World();
	~World();

	void			Initialize();

	void			ProcessInput();
	void			Update(float deltaSeconds);

	Chunk*			GetActiveChunkContainingPosition(const Vector3& position) const;
	RenderScene*	GetRenderScene() const { return m_renderScene; }


private:
	//-----Private Methods-----

	// Chunk activation
	void			CheckToActivateChunks();
	bool			GetClosestInactiveChunkWithinActivationRange(IntVector3& out_closestInactiveChunkCoords) const;
	void			AddChunkToActiveList(Chunk* chunk);

	// Chunk deactivation
	void			CheckToDeactivateChunks();



private:
	//-----Private Data-----

	// Chunks
	std::map<IntVector3, Chunk*>	m_activeChunks;

	// Rendering
	RenderScene*					m_renderScene = nullptr;

	// Static constants
	static constexpr int			SEA_LEVEL = 96;
	static constexpr int			BASE_ELEVATION = 128;
	static constexpr int			NOISE_MAX_DEVIATION_FROM_BASE_ELEVATION = 64;
	static constexpr int			WORLD_MAX_CHUNK_HEIGHT = 4;
	static constexpr float			DEFAULT_CHUNK_ACTIVATION_RANGE = 100.f;
	static constexpr float			DEFAULT_CHUNK_DEACTIVATION_OFFSET = 32.f; // A chunk's worth

};

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------