///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: Jan 17th, 2022
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Block/BlockLocator.h"
#include "Game/Chunk/Chunk.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Math/MathUtils.h"

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
BlockLocator::BlockLocator(Chunk* chunk, uint16 blockIndex)
	: m_chunk(chunk), m_blockIndex(blockIndex)
{
}


//-------------------------------------------------------------------------------------------------
BlockLocator::BlockLocator(Chunk* chunk, const IntVector3& blockCoords)
	: m_chunk(chunk)
{
	m_blockIndex = Chunk::GetBlockIndexForCoords(blockCoords);
}


//-------------------------------------------------------------------------------------------------
Block& BlockLocator::GetBlock() const
{
	if (m_chunk != nullptr)
	{
		return m_chunk->GetBlock(m_blockIndex);
	}

	return Block::MISSING_BLOCK;
}


//-------------------------------------------------------------------------------------------------
IntVector3 BlockLocator::GetBlockCoords() const
{
	return Chunk::GetBlockCoordsForIndex(m_blockIndex);
}


//-------------------------------------------------------------------------------------------------
BlockLocator BlockLocator::ToEast() const
{
	// Don't do anything on an invalid block locator
	if (m_chunk == nullptr)
	{
		return BlockLocator(nullptr, 0);
	}

	// If we're at the east boundary of this chunk, move into the next chunk
	if ((m_blockIndex & Chunk::CHUNK_X_MASK) == Chunk::CHUNK_X_MASK)
	{
		return BlockLocator(nullptr, 0);
		//int newBlockIndex = m_blockIndex & ~Chunk::CHUNK_X_MASK;
		//return BlockLocator(m_chunk->GetEastNeighbor(), newBlockIndex);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex + 1);
	}
}


//-------------------------------------------------------------------------------------------------
BlockLocator BlockLocator::ToWest() const
{
	// Don't do anything on an invalid block locator
	if (m_chunk == nullptr)
	{
		return BlockLocator(nullptr, 0);
	}

	// If we're at the west boundary of this chunk, move into the next chunk
	if ((m_blockIndex & Chunk::CHUNK_X_MASK) == 0)
	{
		return BlockLocator(nullptr, 0);
		//int newBlockIndex = m_blockIndex | Chunk::CHUNK_X_MASK;
		//return BlockLocator(m_chunk->GetWestNeighbor(), newBlockIndex);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex - 1);
	}
}


//-------------------------------------------------------------------------------------------------
BlockLocator BlockLocator::ToNorth() const
{
	// Don't do anything on an invalid block locator
	if (m_chunk == nullptr)
	{
		return BlockLocator(nullptr, 0);
	}

	// If we're at the north boundary of this chunk, move into the next chunk
	if ((m_blockIndex & Chunk::CHUNK_Z_MASK) == Chunk::CHUNK_Z_MASK)
	{
		return BlockLocator(nullptr, 0);
		//int newBlockIndex = m_blockIndex & ~Chunk::CHUNK_Y_MASK;
		//return BlockLocator(m_chunk->GetNorthNeighbor(), newBlockIndex);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex + Chunk::CHUNK_DIMENSIONS_X);
	}
}


//-------------------------------------------------------------------------------------------------
BlockLocator BlockLocator::ToSouth() const
{
	// Don't do anything on an invalid block locator
	if (m_chunk == nullptr)
	{
		return BlockLocator(nullptr, 0);
	}

	// If we're at the south boundary of this chunk, move into the next chunk
	if ((m_blockIndex & Chunk::CHUNK_Z_MASK) == 0)
	{
		return BlockLocator(nullptr, 0);
		//int newBlockIndex = m_blockIndex | Chunk::CHUNK_Y_MASK;
		//return BlockLocator(m_chunk->GetSouthNeighbor(), newBlockIndex);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex - Chunk::CHUNK_DIMENSIONS_X);
	}
}


//-------------------------------------------------------------------------------------------------
BlockLocator BlockLocator::ToAbove() const
{
	// Don't do anything on an invalid block locator
	if (m_chunk == nullptr)
	{
		return BlockLocator(nullptr, 0);
	}

	// If we're at the top of the chunk return a missing block locator
	if ((m_blockIndex & Chunk::CHUNK_Y_MASK) == Chunk::CHUNK_Y_MASK)
	{
		return BlockLocator(nullptr, 0);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex + Chunk::BLOCKS_PER_Y_LAYER);
	}
}


//-------------------------------------------------------------------------------------------------
BlockLocator BlockLocator::ToBelow() const
{
	// Don't do anything on an invalid block locator
	if (m_chunk == nullptr)
	{
		return BlockLocator(nullptr, 0);
	}

	// If we're at the bottom of the chunk return a missing block locator
	if ((m_blockIndex & Chunk::CHUNK_Y_MASK) == 0)
	{
		return BlockLocator(nullptr, 0);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex - Chunk::BLOCKS_PER_Y_LAYER);
	}
}


//-------------------------------------------------------------------------------------------------
BlockLocator BlockLocator::ToCoords(const IntVector3& offset) const
{
	BlockLocator finalResult = *this;

	bool towardsEast = offset.x > 0;
	bool towardsUp = offset.y > 0;
	bool towardsNorth = offset.z > 0;

	IntVector3 numSteps = Abs(offset);

	for (int xStep = 0; xStep < numSteps.x; ++xStep)
	{
		if (towardsEast)
		{
			finalResult = finalResult.ToEast();
		}
		else
		{
			finalResult = finalResult.ToWest();
		}
	}

	for (int yStep = 0; yStep < numSteps.y; ++yStep)
	{
		if (towardsUp)
		{
			finalResult = finalResult.ToAbove();
		}
		else
		{
			finalResult = finalResult.ToBelow();
		}
	}

	for (int zStep = 0; zStep < numSteps.z; ++zStep)
	{
		if (towardsNorth)
		{
			finalResult = finalResult.ToNorth();
		}
		else
		{
			finalResult = finalResult.ToSouth();
		}
	}

	return finalResult;
}


//-------------------------------------------------------------------------------------------------
bool BlockLocator::operator==(const BlockLocator& other) const
{
	return m_blockIndex == other.m_blockIndex && m_chunk == other.m_chunk;
}
