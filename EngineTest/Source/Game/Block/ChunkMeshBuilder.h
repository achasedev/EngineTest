///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: Jan 18th, 2022
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------
#pragma once

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Math/IntVector2.h"
#include "Engine/Render/Mesh/MeshBuilder.h"

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
class Chunk;
class Mesh;
enum ChunkLayerDirection
{
	CHUNK_LAYER_INVALID = -1,
	CHUNK_LAYER_RIGHT,
	CHUNK_LAYER_LEFT,
	CHUNK_LAYER_TOP,
	CHUNK_LAYER_BOTTOM,
	CHUNK_LAYER_FRONT,
	CHUNK_LAYER_BACK,
	NUM_CHUNK_LAYER_DIRECTIONS
};

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
class ChunkMeshBuilder
{
public:
	//-----Public Methods-----

	void BuildMeshForChunk(Chunk* chunk, bool reduceTriangles);


private:
	//-----Private Methods-----

	void			BuildStandardMesh(Mesh& mesh);
	void			PushVerticesForBlock(const IntVector3& blockCoords, const BlockDefinition* def);
	void			PushVerticesForBlock(uint16 blockIndex, const BlockDefinition* def);

	void			BuildReducedMesh(Mesh& mesh);
	void			InitializeCoverForLayer(ChunkLayerDirection direction, uint16 layerIndex);
	bool			IsQuadCovered(const IntVector2& coverCoord) const;
	bool			IsQuadCovered(int coverX, int coverY) const;
	bool			CanQuadContinueRun(const IntVector2& coverCoord, uint16 layerIndex, const Rgba& runColor) const;
	IntVector3		GetBlockCoordsForCoverCoords(IntVector2 coverCoords, uint16 layerIndex) const;
	BlockLocator	GetBlockLocatorForCoverCoords(const IntVector2& coverCoords, uint16 layerIndex) const;
	void			PushFace(const IntVector2& minCoverCoords, const IntVector2& maxCoverCoords, uint16 layerIndex, const Rgba& color);


private:
	//-----Private Data-----

	Chunk*				m_chunk = nullptr;

	// Build state
	bool*				m_cover = nullptr; // M x N values to indicate if a cell is covered
	IntVector2			m_coverDimensions = IntVector2::ZERO;
	ChunkLayerDirection m_direction = CHUNK_LAYER_INVALID;
	MeshBuilder			m_meshBuilder;

};

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------