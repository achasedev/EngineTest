///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: Jan 21st, 2022
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Chunk/Chunk.h"
#include "Game/Chunk/ChunkMeshBuilder.h"
#include "Game/Framework/Game.h"
#include "Game/Framework/GameCommon.h"
#include "Game/Framework/World.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Math/OBB3.h"
#include "Engine/Render/Camera.h"
#include "Engine/Render/Debug/DebugRenderSystem.h"
#include "Engine/Render/RenderScene.h"
#include "Engine/Render/Skybox.h"
#include "Engine/Resource/ResourceSystem.h"

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
World::World()
{
}

//-------------------------------------------------------------------------------------------------
World::~World()
{
	// Delete all active chunks
	std::map<IntVector3, Chunk*>::iterator chunkItr = m_activeChunks.begin();

	for (chunkItr; chunkItr != m_activeChunks.end(); chunkItr++)
	{
		Chunk* chunk = chunkItr->second;

		// TODO: Save/Load
		/*if (chunk->ShouldWriteToFile())
		{
			chunk->WriteToFile();
		}*/

		delete chunk;
	}

	m_activeChunks.clear();
}


//-------------------------------------------------------------------------------------------------
void World::Initialize()
{
	m_renderScene = new RenderScene("World");
	m_renderScene->AddCamera(g_game->GetGameCamera());

	Skybox* skybox = new Skybox(g_resourceSystem->CreateOrGetMaterial("Data/Material/skybox.material"));
	m_renderScene->SetSkybox(skybox);
	m_renderScene->SetAmbience(Rgba(255, 255, 255, 200));

	//// TODO: Remove
	//Chunk* chunk = new Chunk(IntVector3(0, 0, 0));
	//chunk->GenerateWithNoise(16, 10, 12);
	//AddChunkToActiveList(chunk);

	//ChunkMeshBuilder cmb;
	//cmb.BuildMeshForChunk(chunk, true);

	//Renderable rend;
	//Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/chunk.material");
	//rend.AddDraw(chunk->GetMesh(), material);

	//RenderSceneId chunkSceneId = m_renderScene->AddRenderable(rend);
	//chunk->SetRenderSceneId(chunkSceneId);

	//DebugRenderOptions options;
	//options.m_startColor = Rgba::RED;
	//options.m_endColor = Rgba::RED;
	//options.m_fillMode = FILL_MODE_WIREFRAME;
	//options.m_debugRenderMode = DEBUG_RENDER_MODE_XRAY;

	//DebugDrawBox(OBB3(chunk->GetBoundsWs()), options);
}


//-------------------------------------------------------------------------------------------------
void World::ProcessInput()
{
	if (g_inputSystem->WasKeyJustPressed('I'))
	{
		static bool showDebug = false;
		showDebug = !showDebug;

		std::map<IntVector3, Chunk*>::iterator itr = m_activeChunks.begin();
		for (itr; itr != m_activeChunks.end(); itr++)
		{
			Chunk* chunk = itr->second;
			Renderable* rend = m_renderScene->GetRenderable(chunk->GetRenderSceneId());

			if (showDebug)
			{
				Material* debugMat = g_resourceSystem->CreateOrGetMaterial("Data/Material/chunk_debug.material");
				rend->AddDraw(rend->GetDraw(0).m_mesh, debugMat);
			}
			else
			{
				rend->RemoveDraw(1);
			}
		}
	}
}


//-------------------------------------------------------------------------------------------------
void World::Update(float deltaSeconds)
{
	CheckToActivateChunks();

	std::map<IntVector3, Chunk*>::iterator itr = m_activeChunks.begin();
	for (itr; itr != m_activeChunks.end(); itr++)
	{
		Chunk* chunk = itr->second;
		chunk->Update(deltaSeconds);
	}
}


//-------------------------------------------------------------------------------------------------
Chunk* World::GetActiveChunkContainingPosition(const Vector3& position) const
{
	IntVector3 chunkCoords = Chunk::GetChunkCoordsContainingPosition(position);

	bool chunkExists = m_activeChunks.find(chunkCoords) != m_activeChunks.end();
	if (chunkExists)
	{
		return m_activeChunks.at(chunkCoords);
	}

	return nullptr;
}


//-------------------------------------------------------------------------------------------------
void World::CheckToActivateChunks()
{
	IntVector3 closestInactiveChunkCoords;
	bool foundInactiveChunk = GetClosestInactiveChunkWithinActivationRange(closestInactiveChunkCoords);

	if (foundInactiveChunk)
	{
		ConsolePrintf("Activating Chunk (%i, %i, %i)", closestInactiveChunkCoords.x, closestInactiveChunkCoords.y, closestInactiveChunkCoords.z);

		// Populate from data or noise
		// TODO: Save/Load
		Chunk* chunk = new Chunk(closestInactiveChunkCoords);
		chunk->GenerateWithNoise(BASE_ELEVATION, NOISE_MAX_DEVIATION_FROM_BASE_ELEVATION, SEA_LEVEL);

		ChunkMeshBuilder cmb;
		cmb.BuildMeshForChunk(chunk, true);

		Renderable rend;
		Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/chunk.material");
		rend.AddDraw(chunk->GetMesh(), material);

		RenderSceneId chunkSceneId = m_renderScene->AddRenderable(rend);
		chunk->SetRenderSceneId(chunkSceneId);

		// Add to the list
		AddChunkToActiveList(chunk);
	}
}


//-------------------------------------------------------------------------------------------------
bool World::GetClosestInactiveChunkWithinActivationRange(IntVector3& out_closestInactiveChunkCoords) const
{
	float activationRangeSquared = DEFAULT_CHUNK_ACTIVATION_RANGE * DEFAULT_CHUNK_ACTIVATION_RANGE;

	int chunkSpanX = Ceiling(DEFAULT_CHUNK_ACTIVATION_RANGE / (float)Chunk::CHUNK_DIMENSIONS_X);
	int chunkSpanY = Ceiling(DEFAULT_CHUNK_ACTIVATION_RANGE / (float)Chunk::CHUNK_DIMENSIONS_Y);
	int chunkSpanZ = Ceiling(DEFAULT_CHUNK_ACTIVATION_RANGE / (float)Chunk::CHUNK_DIMENSIONS_Z);
	IntVector3 chunkSpan = IntVector3(chunkSpanX, chunkSpanY, chunkSpanZ);

	Vector3 cameraPos = g_game->GetGameCamera()->GetPosition();
	IntVector3 cameraChunkCoords = Chunk::GetChunkCoordsContainingPosition(cameraPos);

	IntVector3 startChunk = cameraChunkCoords - chunkSpan;
	IntVector3 endChunk = cameraChunkCoords + chunkSpan;

	// Clamp to world chunk Z bounds
	startChunk.y = Clamp(startChunk.y, 0, WORLD_MAX_CHUNK_HEIGHT);

	startChunk.x = 0;
	startChunk.z = 0;
	endChunk.x = 1;
	endChunk.z = 1;

	float minDistanceSoFar = activationRangeSquared;
	bool foundInactiveChunk = false;

	for (int y = startChunk.y; y <= endChunk.y; ++y)
	{
		for (int z = startChunk.z; z <= endChunk.z; ++z)
		{
			for (int x = startChunk.x; x <= endChunk.x; ++x)
			{
				IntVector3 currChunkCoords = IntVector3(x, y, z);

				// If the chunk is already active just continue
				if (m_activeChunks.find(currChunkCoords) != m_activeChunks.end())
					continue;
				
				// Get the distance to the chunk
				Vector3 chunkOriginPos = Vector3(currChunkCoords.x * Chunk::CHUNK_DIMENSIONS_X, currChunkCoords.y * Chunk::CHUNK_DIMENSIONS_Y, currChunkCoords.z * Chunk::CHUNK_DIMENSIONS_Z);
				Vector3 chunkCenter = chunkOriginPos + 0.5f * Vector3(Chunk::CHUNK_DIMENSIONS_X, Chunk::CHUNK_DIMENSIONS_Y, Chunk::CHUNK_DIMENSIONS_Z);
				Vector3 vectorToChunkCenter = (chunkCenter - cameraPos);

				float distanceSquared = vectorToChunkCenter.GetLengthSquared();

				// Update our min if it's smaller
				if (distanceSquared < minDistanceSoFar)
				{
					minDistanceSoFar = distanceSquared;
					out_closestInactiveChunkCoords = currChunkCoords;
					foundInactiveChunk = true;
				}
			}
		}
	}

	return foundInactiveChunk;
}


//-------------------------------------------------------------------------------------------------
void World::AddChunkToActiveList(Chunk* chunk)
{
	// Check for duplicates
	IntVector3 chunkCoords = chunk->GetChunkCoords();
	bool alreadyExists = m_activeChunks.find(chunkCoords) != m_activeChunks.end();
	ASSERT_OR_DIE(!alreadyExists, Stringf("World attempted to add duplicate chunk at coords (%i, %i)", chunkCoords.x, chunkCoords.y).c_str());

	// Add it to the map
	m_activeChunks[chunkCoords] = chunk;

	// Hook up the references to the neighbors
	IntVector3 eastCoords = chunkCoords + IntVector3(1, 0, 0);
	IntVector3 westCoords = chunkCoords + IntVector3(-1, 0, 0);
	IntVector3 northCoords = chunkCoords + IntVector3(0, 1, 0);
	IntVector3 southCoords = chunkCoords + IntVector3(0, -1, 0);
	IntVector3 aboveCoords = chunkCoords + IntVector3(0, 0, 1);
	IntVector3 belowCoords = chunkCoords + IntVector3(0, 0, -1);

	bool eastExists = m_activeChunks.find(eastCoords) != m_activeChunks.end();
	bool westExists = m_activeChunks.find(westCoords) != m_activeChunks.end();
	bool northExists = m_activeChunks.find(northCoords) != m_activeChunks.end();
	bool southExists = m_activeChunks.find(southCoords) != m_activeChunks.end();
	bool aboveExists = m_activeChunks.find(aboveCoords) != m_activeChunks.end();
	bool belowExists = m_activeChunks.find(belowCoords) != m_activeChunks.end();

	if (eastExists)
	{
		Chunk* eastChunk = m_activeChunks[eastCoords];

		chunk->SetEastNeighbor(eastChunk);
		eastChunk->SetWestNeighbor(chunk);
	}

	if (westExists)
	{
		Chunk* westChunk = m_activeChunks[westCoords];

		chunk->SetWestNeighbor(westChunk);
		westChunk->SetEastNeighbor(chunk);
	}

	if (northExists)
	{
		Chunk* northChunk = m_activeChunks[northCoords];

		chunk->SetNorthNeighbor(northChunk);
		northChunk->SetSouthNeighbor(chunk);
	}

	if (southExists)
	{
		Chunk* southChunk = m_activeChunks[southCoords];

		chunk->SetSouthNeighbor(southChunk);
		southChunk->SetNorthNeighbor(chunk);
	}

	if (aboveExists)
	{
		Chunk* aboveChunk = m_activeChunks[aboveCoords];

		chunk->SetSouthNeighbor(aboveChunk);
		aboveChunk->SetNorthNeighbor(chunk);
	}

	if (belowExists)
	{
		Chunk* belowChunk = m_activeChunks[belowCoords];

		chunk->SetSouthNeighbor(belowChunk);
		belowChunk->SetNorthNeighbor(chunk);
	}
}
