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
#include "Game/Block/BlockIterator.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Math/AABB3.h"
#include "Engine/Math/IntVector3.h"

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

	void			GenerateWithNoise(int baseElevation, int maxDeviationFromBaseElevation, int seaLevel);
	void			BuildMesh();

	void			Update();
	void			Render();

	void			GetWorldCenter() const;
	void			GetWorldBounds() const;
	BlockIterator	GetBlockContainingPosition(const Vector3& worldPosition);
	Block&			GetBlock(int blockIndex);
	Block&			GetBlock(const IntVector3& blockCoords);
	IntVector3		GetChunkCoords() const { return m_chunkCoords; }
	Mesh*			GetMesh() const { return m_mesh; }

	void			SetBlockDefinition(int blockIndex, const BlockDefinition* definition);
	void			SetBlockDefinition(const IntVector3& blockCoords, const BlockDefinition* definition);

	static IntVector3	GetBlockCoordsForIndex(int blockIndex);
	static int			GetBlockIndexForCoords(const IntVector3& coords);


public:
	//-----Public Data-----

	static constexpr uint8	CHUNK_VERSION = 1;

	static constexpr uint8	CHUNK_BITS_X = 5;											// Number of bits in the Block Index to represent the x coordinate
	static constexpr uint8	CHUNK_BITS_Y = 5;											// Number of bits in the Block Index to represent the y coordinate
	static constexpr uint8	CHUNK_BITS_Z = 5;											// Number of bits in the Block Index to represent the z coordinate
	static constexpr uint8	CHUNK_BITS_XZ = CHUNK_BITS_X + CHUNK_BITS_Z;				// Number of bits in the Block Index for both x and z

	static constexpr int	CHUNK_DIMENSIONS_X = (1 << CHUNK_BITS_X);
	static constexpr int	CHUNK_DIMENSIONS_Y = (1 << CHUNK_BITS_Y);
	static constexpr int	CHUNK_DIMENSIONS_Z = (1 << CHUNK_BITS_Z);

	static constexpr int	CHUNK_X_MASK = CHUNK_DIMENSIONS_X - 1;						// Mask that when applied to a BlockIndex will give the block's X coordinate
	static constexpr int	CHUNK_Y_MASK = (CHUNK_DIMENSIONS_Y - 1) << CHUNK_BITS_XZ;	// Mask that when applied to a BlockIndex will give the block's Y coordinate
	static constexpr int	CHUNK_Z_MASK = (CHUNK_DIMENSIONS_Z - 1) << CHUNK_BITS_X;	// Mask that when applied to a BlockIndex will give the block's Z coordinate

	static constexpr int	BLOCKS_PER_Z_ROW = CHUNK_DIMENSIONS_X;
	static constexpr int	BLOCKS_PER_Y_LAYER = CHUNK_DIMENSIONS_X * CHUNK_DIMENSIONS_Z;
	static constexpr int	BLOCKS_PER_CHUNK = CHUNK_DIMENSIONS_X * CHUNK_DIMENSIONS_Y * CHUNK_DIMENSIONS_Z;


private:
	//-----Private Methods-----

	void PushVerticesForBlock(const IntVector3& blockCoords, const BlockDefinition* def, MeshBuilder& mb);
	void PushVerticesForBlock(int blockIndex, const BlockDefinition* def, MeshBuilder& mb);


private:
	//-----Private Data-----

	IntVector3	m_chunkCoords = IntVector3::ZERO;
	AABB3		m_worldBounds = AABB3(Vector3::ZERO, Vector3::ZERO);
	Block		m_blocks[BLOCKS_PER_CHUNK];

	Mesh*		m_mesh = nullptr;

};

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------