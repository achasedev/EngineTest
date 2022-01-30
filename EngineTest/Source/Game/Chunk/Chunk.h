///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: Jan 17th, 2022
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------
#pragma once

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Block/Block.h"
#include "Game/Block/BlockLocator.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Math/AABB3.h"
#include "Engine/Math/IntVector3.h"
#include "Engine/Render/Renderable.h"

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------
class Mesh;
class MeshBuilder;

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
class Chunk
{
public:
	//-----Public Methods-----

	Chunk(const IntVector3& chunkCoords);
	~Chunk();

	void				GenerateWithNoise(int baseElevation, int maxDeviationFromBaseElevation, int seaLevel);

	Mesh*				CreateOrGetMesh();
	void				Update(float deltaSeconds);

	Vector3				GetOriginWs() const { return m_worldBounds.mins; }
	Vector3				GetCenterWs() const { return m_worldBounds.GetCenter(); }
	AABB3				GetBoundsWs() const { return m_worldBounds; }
	BlockLocator		GetBlockContainingPosition(const Vector3& worldPosition);
	Block&				GetBlock(uint16 blockIndex);
	Block&				GetBlock(const IntVector3& blockCoords);
	IntVector3			GetChunkCoords() const { return m_chunkCoords; }
	Mesh*				GetMesh() const { return m_mesh; }
	RenderSceneId		GetRenderSceneId() const { return m_renderSceneId; }
	Chunk*				GetEastNeighbor() const { return m_eastNeighborChunk; }
	Chunk*				GetWestNeighbor() const { return m_westNeighborChunk; }
	Chunk*				GetNorthNeighbor() const { return m_northNeighborChunk; }
	Chunk*				GetSouthNeighbor() const { return m_southNeighborChunk; }
	Chunk*				GetAboveNeighbor() const { return m_aboveNeighborChunk; }
	Chunk*				GetBelowNeighbor() const { return m_belowNeighborChunk; }
	bool				IsAllAir() const;

	void				SetRenderSceneId(RenderSceneId id) { m_renderSceneId = id; }
	void				SetBlockDefinition(uint16 blockIndex, const BlockDefinition* definition);
	void				SetBlockDefinition(const IntVector3& blockCoords, const BlockDefinition* definition);
	void				SetEastNeighbor(Chunk* chunkToEast) { m_eastNeighborChunk = chunkToEast; }
	void				SetWestNeighbor(Chunk* chunkToWest) { m_westNeighborChunk = chunkToWest; }
	void				SetNorthNeighbor(Chunk* chunkToNorth) { m_northNeighborChunk = chunkToNorth; }
	void				SetSouthNeighbor(Chunk* chunkToSouth) { m_southNeighborChunk = chunkToSouth; }
	void				SetAboveNeighbor(Chunk* chunkAbove) { m_aboveNeighborChunk = chunkAbove; }
	void				SetBelowNeighbor(Chunk* chunkBelow) { m_belowNeighborChunk = chunkBelow; }

	static bool			AreBlockCoordsValid(const IntVector3& coords);
	static IntVector3	GetBlockCoordsForIndex(uint16 blockIndex);
	static uint16		GetBlockIndexForCoords(const IntVector3& coords);
	static IntVector3	GetChunkCoordsContainingPosition(const Vector3& position);


public:
	//-----Public Data-----

	static constexpr uint8	CHUNK_VERSION = 1;

	static constexpr uint8	CHUNK_BITS_X = 5;											// Number of bits in the Block Index to represent the x coordinate
	static constexpr uint8	CHUNK_BITS_Y = 6;											// Number of bits in the Block Index to represent the y coordinate
	static constexpr uint8	CHUNK_BITS_Z = 5;											// Number of bits in the Block Index to represent the z coordinate
	static constexpr uint8	CHUNK_BITS_XZ = CHUNK_BITS_X + CHUNK_BITS_Z;				// Number of bits in the Block Index for both x and z

	static constexpr uint16	CHUNK_DIMENSIONS_X = (1 << CHUNK_BITS_X);
	static constexpr uint16	CHUNK_DIMENSIONS_Y = (1 << CHUNK_BITS_Y);
	static constexpr uint16	CHUNK_DIMENSIONS_Z = (1 << CHUNK_BITS_Z);

	static constexpr uint16	CHUNK_X_MASK = (uint16)CHUNK_DIMENSIONS_X - 1;						// Mask that when applied to a BlockIndex will give the block's X coordinate
	static constexpr uint16	CHUNK_Y_MASK = (uint16)((CHUNK_DIMENSIONS_Y - 1) << CHUNK_BITS_XZ);	// Mask that when applied to a BlockIndex will give the block's Y coordinate
	static constexpr uint16	CHUNK_Z_MASK = (uint16)((CHUNK_DIMENSIONS_Z - 1) << CHUNK_BITS_X);	// Mask that when applied to a BlockIndex will give the block's Z coordinate

	static constexpr uint32	BLOCKS_PER_Z_ROW = CHUNK_DIMENSIONS_X;
	static constexpr uint32	BLOCKS_PER_Y_LAYER = CHUNK_DIMENSIONS_X * CHUNK_DIMENSIONS_Z;
	static constexpr uint32	BLOCKS_PER_CHUNK = CHUNK_DIMENSIONS_X * CHUNK_DIMENSIONS_Y * CHUNK_DIMENSIONS_Z;


private:
	//-----Private Data-----

	IntVector3		m_chunkCoords = IntVector3::ZERO;
	AABB3			m_worldBounds = AABB3(Vector3::ZERO, Vector3::ZERO);
	Block			m_blocks[BLOCKS_PER_CHUNK];

	Chunk*			m_eastNeighborChunk = nullptr;
	Chunk*			m_westNeighborChunk = nullptr;
	Chunk*			m_northNeighborChunk = nullptr;
	Chunk*			m_southNeighborChunk = nullptr;
	Chunk*			m_aboveNeighborChunk = nullptr;
	Chunk*			m_belowNeighborChunk = nullptr;

	Mesh*			m_mesh = nullptr;
	RenderSceneId	m_renderSceneId = INVALID_RENDER_SCENE_ID;

};

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------