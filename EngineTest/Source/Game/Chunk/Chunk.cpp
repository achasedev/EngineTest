///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: Jan 17th, 2022
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Block/BlockDefinition.h"
#include "Game/Chunk/Chunk.h"
#include "Game/Chunk/ChunkMeshBuilder.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/Vector2.h"
#include "Engine/Render/Mesh/MeshBuilder.h"
#include "ThirdParty/squirrel/SmoothNoise.hpp"

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
Chunk::Chunk(const IntVector3& chunkCoords)
	: m_chunkCoords(chunkCoords)
{
	// Calculate world bounds
	m_worldBounds.mins = Vector3(chunkCoords.x * CHUNK_DIMENSIONS_X, chunkCoords.y * CHUNK_DIMENSIONS_Y, chunkCoords.z * CHUNK_DIMENSIONS_Z);
	m_worldBounds.maxs = m_worldBounds.mins + Vector3(CHUNK_DIMENSIONS_X, CHUNK_DIMENSIONS_Y, CHUNK_DIMENSIONS_Z);
}


//-------------------------------------------------------------------------------------------------
Chunk::~Chunk()
{
	SAFE_DELETE(m_mesh);
}


//-------------------------------------------------------------------------------------------------
void Chunk::GenerateWithNoise(int baseElevation, int maxDeviationFromBaseElevation, int seaLevel)
{
	const BlockDefinition* grassDef = BlockDefinition::GetDefinition(SID("grass"));
	const BlockDefinition* dirtDef = BlockDefinition::GetDefinition(SID("dirt"));
	const BlockDefinition* stoneDef = BlockDefinition::GetDefinition(SID("stone"));
	const BlockDefinition* waterDef = BlockDefinition::GetDefinition(SID("water"));
	const BlockDefinition* airDef = BlockDefinition::GetDefinition(SID("air"));

	for (int zIndex = 0; zIndex < CHUNK_DIMENSIONS_Z; ++zIndex)
	{
		for (int xIndex = 0; xIndex < CHUNK_DIMENSIONS_X; ++xIndex)
		{
			// Use XY center positions for the noise, in world coordinates
			Vector3 chunkOffsetWs = Vector3(m_chunkCoords.x * CHUNK_DIMENSIONS_X, m_chunkCoords.y * CHUNK_DIMENSIONS_Y, m_chunkCoords.z * CHUNK_DIMENSIONS_Z);
			Vector2 blockCenterWs = Vector2(chunkOffsetWs.x, chunkOffsetWs.z) + Vector2(xIndex, zIndex) + Vector2(0.5f);

			// Get the height of the chunk at these coordinates
			float noise = Compute2dPerlinNoise(blockCenterWs.x, blockCenterWs.y, 50.f);
			int elevationFromNoise = RoundToNearestInt(noise * maxDeviationFromBaseElevation) + baseElevation;

			int maxHeightForThisColumn = Max(elevationFromNoise, seaLevel);

			for (int yIndex = 0; yIndex < CHUNK_DIMENSIONS_Y; ++yIndex)
			{
				const BlockDefinition* typeToUse = nullptr;

				if (yIndex > maxHeightForThisColumn - 1)
				{
					typeToUse = airDef;
				}
				else if (elevationFromNoise >= seaLevel)
				{
					if (yIndex == elevationFromNoise - 1)
					{
						typeToUse = grassDef;
					}
					else if (yIndex > elevationFromNoise - 4) // Dirt for 3 blocks below the grass height
					{
						typeToUse = dirtDef;
					}
					else
					{
						typeToUse = stoneDef;
					}
				}
				else
				{
					if (yIndex >= elevationFromNoise)
					{
						typeToUse = waterDef;
					}
					else if (yIndex > seaLevel - 4)
					{
						typeToUse = dirtDef;
					}
					else
					{
						typeToUse = stoneDef;
					}
				}


				IntVector3 blockCoords = IntVector3(xIndex, yIndex, zIndex);
				SetBlockDefinition(blockCoords, typeToUse);
			}
		}
	}
}


//-------------------------------------------------------------------------------------------------
void Chunk::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}


//-------------------------------------------------------------------------------------------------
Block& Chunk::GetBlock(const IntVector3& blockCoords)
{
	uint16 blockIndex = Chunk::GetBlockIndexForCoords(blockCoords);
	return GetBlock(blockIndex);
}


//-------------------------------------------------------------------------------------------------
Block& Chunk::GetBlock(uint16 blockIndex)
{
	return m_blocks[blockIndex];
}


//-------------------------------------------------------------------------------------------------
Mesh* Chunk::CreateOrGetMesh()
{
	if (m_mesh == nullptr)
	{
		m_mesh = new Mesh();
	}

	return m_mesh;
}


//-------------------------------------------------------------------------------------------------
void Chunk::SetBlockDefinition(uint16 blockIndex, const BlockDefinition* definition)
{
	Block& block = m_blocks[blockIndex];
	block.SetDefinition(definition);

	// TODO: lighting
	// Dirty the mesh and adjacent neighbors if the block was on the XY-border of the chunk
	//m_isMeshDirty = true;

	//IntVector3 blockCoords = GetBlockCoordsFromBlockIndex(blockIndex);

	//if (blockCoords.x == 0 && m_westNeighborChunk != nullptr) // West Neighbor
	//{
	//	m_westNeighborChunk->SetIsMeshDirty(true);
	//}
	//else if (blockCoords.x == CHUNK_DIMENSIONS_X - 1 && m_eastNeighborChunk != nullptr) // East Neighbor
	//{
	//	m_eastNeighborChunk->SetIsMeshDirty(true);
	//}

	//if (blockCoords.y == 0 && m_southNeighborChunk != nullptr) // South Neighbor
	//{
	//	m_southNeighborChunk->SetIsMeshDirty(true);
	//}
	//else if (blockCoords.y == CHUNK_DIMENSIONS_Y - 1 && m_northNeighborChunk != nullptr) // North Neighbor
	//{
	//	m_northNeighborChunk->SetIsMeshDirty(true);
	//}
}


//-------------------------------------------------------------------------------------------------
void Chunk::SetBlockDefinition(const IntVector3& blockCoords, const BlockDefinition* definition)
{
	uint16 blockIndex = GetBlockIndexForCoords(blockCoords);
	SetBlockDefinition(blockIndex, definition);
}


//-------------------------------------------------------------------------------------------------
bool Chunk::AreBlockCoordsValid(const IntVector3& coords)
{
	if (coords.x < 0 || coords.y < 0 || coords.z < 0 || coords.x >= CHUNK_DIMENSIONS_X || coords.y > CHUNK_DIMENSIONS_Y || coords.z > CHUNK_DIMENSIONS_Z)
		return false;

	return true;
}


//-------------------------------------------------------------------------------------------------
IntVector3 Chunk::GetBlockCoordsForIndex(uint16 blockIndex)
{
	int xCoord = blockIndex & CHUNK_X_MASK;
	int zCoord = (blockIndex & CHUNK_Z_MASK) >> CHUNK_BITS_X;
	int yCoord = (blockIndex & CHUNK_Y_MASK) >> CHUNK_BITS_XZ;

	return IntVector3(xCoord, yCoord, zCoord);
}


//-------------------------------------------------------------------------------------------------
uint16 Chunk::GetBlockIndexForCoords(const IntVector3& coords)
{
	ASSERT_OR_DIE(AreBlockCoordsValid(coords), "Invalid coords!");
	return (uint16)(BLOCKS_PER_Y_LAYER * coords.y + BLOCKS_PER_Z_ROW * coords.z + coords.x);
}
