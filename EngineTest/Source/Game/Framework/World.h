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
	Chunk*			GetFurthestActiveChunkOutsideActivationRange() const;
	void			RemoveChunkFromActiveList(Chunk* chunk);


private:
	//-----Private Data-----

	// Chunks
	std::map<IntVector3, Chunk*>	m_activeChunks;

	// Rendering
	RenderScene*					m_renderScene = nullptr;
	ChunkMeshType					m_chunkMeshType = CHUNK_MESH_OPTIMIZED;

	// Static constants
	static constexpr int			SEA_LEVEL = 0;
	static constexpr int			BASE_ELEVATION = 64;
	static constexpr int			NOISE_MAX_DEVIATION_FROM_BASE_ELEVATION = 0;
	static constexpr int			WORLD_MAX_CHUNK_HEIGHT = 4;
	static constexpr float			DEFAULT_CHUNK_ACTIVATION_RANGE = 128.f;
	static constexpr float			DEFAULT_CHUNK_DEACTIVATION_OFFSET = 32.f; // 1 chunk's worth

};

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------