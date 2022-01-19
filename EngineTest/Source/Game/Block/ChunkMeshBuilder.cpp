///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: Jan 18, 2022
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Block/Block.h"
#include "Game/Block/BlockLocator.h"
#include "Game/Block/Chunk.h"
#include "Game/Block/ChunkMeshBuilder.h"
#include "Engine/Core/DevConsole.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Math/IntVector3.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Render/Mesh/MeshBuilder.h"

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

//-------------------------------------------------------------------------------------------------
static bool IsBlockFaceVisible(const BlockLocator& loc, ChunkLayerDirection dir)
{
	if (loc.GetBlock().GetColor().a == 0)
		return false;

	BlockLocator frontLoc;

	switch (dir)
	{

	case CHUNK_LAYER_RIGHT:
		frontLoc = loc.ToEast();
		break;
	case CHUNK_LAYER_LEFT:
		frontLoc = loc.ToWest();
		break;
	case CHUNK_LAYER_TOP:
		frontLoc = loc.ToAbove();
		break;
	case CHUNK_LAYER_BOTTOM:
		frontLoc = loc.ToBelow();
		break;
	case CHUNK_LAYER_FRONT:
		frontLoc = loc.ToNorth();
		break;
	case CHUNK_LAYER_BACK:
		frontLoc = loc.ToSouth();
		break;
	case CHUNK_LAYER_INVALID:
	case NUM_CHUNK_LAYER_DIRECTIONS:
	default:
		ERROR_AND_DIE("Bad enum!");
		break;
	}

	return !frontLoc.GetBlock().IsOpaque();
}

//-------------------------------------------------------------------------------------------------
static uint16 GetNumLayersForDirection(ChunkLayerDirection direction)
{
	switch (direction)
	{

	case CHUNK_LAYER_RIGHT:
	case CHUNK_LAYER_LEFT:
		return Chunk::CHUNK_DIMENSIONS_X;
		break;
	case CHUNK_LAYER_TOP:
	case CHUNK_LAYER_BOTTOM:
		return Chunk::CHUNK_DIMENSIONS_Y;
		break;
	case CHUNK_LAYER_FRONT:
	case CHUNK_LAYER_BACK:
		return Chunk::CHUNK_DIMENSIONS_Z;
		break;
	case CHUNK_LAYER_INVALID:
	case NUM_CHUNK_LAYER_DIRECTIONS:
	default:
		ERROR_AND_DIE("Bad direction!");
		break;
	}
}


///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS IMPLEMENTATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
ChunkMeshBuilder::ChunkMeshBuilder(Chunk* chunk)
	: m_chunk(chunk)
{
}


//-------------------------------------------------------------------------------------------------
void ChunkMeshBuilder::BuildMesh(Mesh& mesh)
{
	m_meshBuilder.Clear();
	m_meshBuilder.BeginBuilding(TOPOLOGY_TRIANGLE_LIST, true);

	for (int iLayerDir = 0; iLayerDir < NUM_CHUNK_LAYER_DIRECTIONS; ++iLayerDir)
	{
		ChunkLayerDirection direction = (ChunkLayerDirection)iLayerDir;
		uint16 numLayers = GetNumLayersForDirection(direction);

		for (uint16 layerIndex = 0; layerIndex < numLayers; ++layerIndex)
		{
			InitializeCoverForLayer(direction, layerIndex);

			for (int startY = 0; startY < m_coverDimensions.y; ++startY)
			{
				for (int startX = 0; startX < m_coverDimensions.x; ++startX)
				{
					IntVector2 startCoverCoords(startX, startY);

					// If we already covered this/it's not visible, move on
					if (IsQuadCovered(startCoverCoords))
						continue;

					IntVector3 startBlockCoords = GetBlockCoordsForCoverCoords(startCoverCoords, layerIndex);
					BlockLocator startBlockLoc(m_chunk, startBlockCoords);
					Rgba runColor = startBlockLoc.GetBlock().GetColor();

					IntVector2 bestMaxCoordsInclusive(-1, -1);
					int lowestYThisRun = 9999999;
					int bestArea = -1;
					for (int runX = startX; runX < m_coverDimensions.x; ++runX) // Start on coverX so we can get the height for the starting column too
					{
						IntVector2 horizCoverCoords(runX, startY);

						if (runX != startX) // (Don't check the first one if it's covered, we already checked that)
						{
							if (!CanQuadContinueRun(horizCoverCoords, layerIndex, runColor))
								break;
						}

						// Find the max height for this "column" now, checking both for cover/visibility and color
						int maxYForThisX = startY;
						for (int runY = startY; runY < m_coverDimensions.y; ++runY)
						{
							IntVector2 vertCoverCoords(runX, runY);

							if (!CanQuadContinueRun(vertCoverCoords, layerIndex, runColor))
								break;

							// It's visible, not covered, and is the same color, so mark the height for this column
							maxYForThisX = runY;
						}

						// This Y can't be greater than the lowest Y so far, so take min
						maxYForThisX = Min(maxYForThisX, lowestYThisRun);
						lowestYThisRun = maxYForThisX;

						// Now update our best area if this is better
						int currentArea = (runX - startX + 1) * (maxYForThisX - startY + 1);
						if (currentArea > bestArea)
						{
							bestArea = currentArea;
							bestMaxCoordsInclusive = IntVector2(runX, maxYForThisX);
						}
					}

					// Create the quad to be pushed
					PushFace(startCoverCoords, bestMaxCoordsInclusive, layerIndex, runColor);

					// Mark these quads as covered
					for (int runX = startX; runX <= bestMaxCoordsInclusive.x; ++runX)
					{
						for (int runY = startY; runY <= bestMaxCoordsInclusive.y; ++runY)
						{
							int index = runY * m_coverDimensions.x + runX;
							m_cover[index] = true;
						}
					}

					// Update our moving cover coords to the next starting point 
					startX = bestMaxCoordsInclusive.x;
				}
			}
		} // For each layer
	} // For each direction

	m_meshBuilder.FinishBuilding();
	m_meshBuilder.UpdateMesh<Vertex3D_PCU>(mesh);

	int numTris = m_meshBuilder.GetIndexCount() / 3;
	int numVerts = m_meshBuilder.GetVertexCount();

	ConsolePrintf(Rgba::CYAN, 300.f, "Vertices: %i - Triangles: %i", numVerts, numTris);
}


//-------------------------------------------------------------------------------------------------
void ChunkMeshBuilder::InitializeCoverForLayer(ChunkLayerDirection direction, uint16 layerIndex)
{
	SAFE_FREE(m_cover);
	m_direction = direction;

	switch (direction)
	{
	case CHUNK_LAYER_RIGHT:
	case CHUNK_LAYER_LEFT:
		m_coverDimensions = IntVector2(Chunk::CHUNK_DIMENSIONS_Z, Chunk::CHUNK_DIMENSIONS_Y);
		break;
	case CHUNK_LAYER_TOP:
	case CHUNK_LAYER_BOTTOM:
		m_coverDimensions = IntVector2(Chunk::CHUNK_DIMENSIONS_X, Chunk::CHUNK_DIMENSIONS_Z);
		break;
	case CHUNK_LAYER_FRONT:
	case CHUNK_LAYER_BACK:
		m_coverDimensions = IntVector2(Chunk::CHUNK_DIMENSIONS_X, Chunk::CHUNK_DIMENSIONS_Y);
		break;
	default:
		ERROR_AND_DIE("Bad direction?");
		break;
	}

	int numBlocks = m_coverDimensions.x * m_coverDimensions.y;
	m_cover = (bool*)malloc(numBlocks * sizeof(bool));
	memset(m_cover, true, numBlocks * sizeof(bool));

	bool visibleFound = false;
	for (int coverY = 0; coverY < m_coverDimensions.y; ++coverY)
	{
		for (int coverX = 0; coverX < m_coverDimensions.x; ++coverX)
		{
			IntVector2 coverCoords = IntVector2(coverX, coverY);
			IntVector3 blockCoords = GetBlockCoordsForCoverCoords(coverCoords, layerIndex);
			BlockLocator blockLoc(m_chunk, blockCoords);

			if (IsBlockFaceVisible(blockLoc, m_direction))
			{
				int coverIndex = m_coverDimensions.x * coverY + coverX;
				m_cover[coverIndex] = false;
				visibleFound = true;
			}
		}
	}

	// Optimization - if there's literally nothing visible here, then just move on
	if (!visibleFound)
	{
		SAFE_FREE(m_cover);
		m_direction = CHUNK_LAYER_INVALID;
		m_coverDimensions = IntVector2::ZERO;
	}
}


//-------------------------------------------------------------------------------------------------
bool ChunkMeshBuilder::IsQuadCovered(const IntVector2& coverCoord) const
{
	return IsQuadCovered(coverCoord.x, coverCoord.y);
}


//-------------------------------------------------------------------------------------------------
bool ChunkMeshBuilder::IsQuadCovered(int coverX, int coverY) const
{
	int index = coverY * m_coverDimensions.x + coverX;
	return m_cover[index];
}


//-------------------------------------------------------------------------------------------------
bool ChunkMeshBuilder::CanQuadContinueRun(const IntVector2& coverCoord, uint16 layerIndex, const Rgba& runColor) const
{
	// If it's already covered, it cannot continue a run
	if (IsQuadCovered(coverCoord))
		return false;

	// If it doesn't match the current running color, it can't continue a run
	BlockLocator blockLoc = GetBlockLocatorForCoverCoords(coverCoord, layerIndex);
	if (blockLoc.GetBlock().GetColor() != runColor)
		return false;

	return true;
}


//-------------------------------------------------------------------------------------------------
IntVector3 ChunkMeshBuilder::GetBlockCoordsForCoverCoords(IntVector2 coverCoords, uint16 layerIndex) const
{
	IntVector3 blockCoords;

	switch (m_direction)
	{
	case CHUNK_LAYER_RIGHT:	
		blockCoords = IntVector3(Chunk::CHUNK_DIMENSIONS_X - 1 - layerIndex, 0, 0) + IntVector3(0, coverCoords.y, coverCoords.x); 
		break;
	case CHUNK_LAYER_LEFT:	
		blockCoords = IntVector3(layerIndex, 0, Chunk::CHUNK_DIMENSIONS_Z - 1) + IntVector3(0, coverCoords.y, -coverCoords.x);
		break;
	case CHUNK_LAYER_TOP:		
		blockCoords = IntVector3(0, Chunk::CHUNK_DIMENSIONS_Y - 1 - layerIndex, 0) + IntVector3(coverCoords.x, 0, coverCoords.y);
		break;
	case CHUNK_LAYER_BOTTOM:	
		blockCoords = IntVector3(0, layerIndex, Chunk::CHUNK_DIMENSIONS_Z - 1) + IntVector3(coverCoords.x, 0, -coverCoords.y);
		break;
	case CHUNK_LAYER_FRONT:	
		blockCoords = IntVector3(Chunk::CHUNK_DIMENSIONS_X -  1, 0, Chunk::CHUNK_DIMENSIONS_Z - 1 - layerIndex) + IntVector3(-coverCoords.x, coverCoords.y, 0);
		break;
	case CHUNK_LAYER_BACK:	
		blockCoords = IntVector3(coverCoords.x, coverCoords.y, layerIndex);
		break;
	default:
		ERROR_RECOVERABLE("Bad direction?");
		break;
	}

	return blockCoords;
}


//-------------------------------------------------------------------------------------------------
BlockLocator ChunkMeshBuilder::GetBlockLocatorForCoverCoords(const IntVector2& coverCoords, uint16 layerIndex) const
{
	IntVector3 blockCoords = GetBlockCoordsForCoverCoords(coverCoords, layerIndex);
	return BlockLocator(m_chunk, blockCoords);
}


//-------------------------------------------------------------------------------------------------
void ChunkMeshBuilder::PushFace(const IntVector2& minCoverCoords, const IntVector2& maxCoverCoords, uint16 layerIndex, const Rgba& color)
{
	IntVector3 minBlockCoords = GetBlockCoordsForCoverCoords(minCoverCoords, layerIndex);
	IntVector3 maxBlockCoords = GetBlockCoordsForCoverCoords(maxCoverCoords, layerIndex);

	Vector3 bottomLeft(minBlockCoords);
	Vector3 topRight(maxBlockCoords);

	Vector3 topLeft, bottomRight;
	float blockSize = Block::BLOCK_SIZE;

	switch (m_direction)
	{
	case CHUNK_LAYER_RIGHT:
		bottomLeft += Vector3(blockSize, 0.f, 0.f);
		topRight += Vector3(blockSize);

		topLeft = Vector3(bottomLeft.x, topRight.y, bottomLeft.z);
		bottomRight = Vector3(topRight.x, bottomLeft.y, topRight.z);
		break;
	case CHUNK_LAYER_LEFT:
		bottomLeft += Vector3(0.f, 0.f, blockSize);
		topRight += Vector3(0.f, blockSize, 0.f);

		topLeft = Vector3(bottomLeft.x, topRight.y, bottomLeft.z);
		bottomRight = Vector3(topRight.x, bottomLeft.y, topRight.z);
		break;
	case CHUNK_LAYER_TOP:
		bottomLeft += Vector3(0.f, blockSize, 0.f);
		topRight += Vector3(blockSize);

		topLeft = Vector3(bottomLeft.x, bottomLeft.y, topRight.z);
		bottomRight = Vector3(topRight.x, topRight.y, bottomLeft.z);
		break;
	case CHUNK_LAYER_BOTTOM:
		bottomLeft += Vector3(0.f, 0.f, blockSize);
		topRight += Vector3(blockSize, 0.f, 0.f);

		topLeft = Vector3(bottomLeft.x, topRight.y, topRight.z);
		bottomRight = Vector3(topRight.x, bottomLeft.y, bottomLeft.z);
		break;
	case CHUNK_LAYER_FRONT:
		bottomLeft += Vector3(blockSize, 0.f, blockSize);
		topRight += Vector3(0.f, blockSize, blockSize);

		topLeft = Vector3(bottomLeft.x, topRight.y, bottomLeft.z);
		bottomRight = Vector3(topRight.x, bottomLeft.y, topRight.z);
		break;
	case CHUNK_LAYER_BACK:
		topRight += Vector3(blockSize, blockSize, 0.f);

		topLeft = Vector3(bottomLeft.x, topRight.y, bottomLeft.z);
		bottomRight = Vector3(topRight.x, bottomLeft.y, topRight.z);
		break;
	default:
		ERROR_AND_DIE("Bad direction!");
		break;
	}

	m_meshBuilder.PushQuad3D(bottomLeft, topLeft, topRight, bottomRight, AABB2::ZERO_TO_ONE, color);
}
