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

	// TODO: Remove
	Chunk* chunk = new Chunk(IntVector3(0, 0, 0));
	chunk->GenerateWithNoise(16, 10, 12);
	AddChunkToActiveList(chunk);

	ChunkMeshBuilder cmb;
	cmb.BuildMeshForChunk(chunk, true);

	Renderable rend;
	Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/chunk.material");
	rend.AddDraw(chunk->GetMesh(), material);

	RenderSceneId chunkSceneId = m_renderScene->AddRenderable(rend);
	chunk->SetRenderSceneId(chunkSceneId);

	DebugRenderOptions options;
	options.m_startColor = Rgba::RED;
	options.m_endColor = Rgba::RED;
	options.m_fillMode = FILL_MODE_WIREFRAME;
	options.m_debugRenderMode = DEBUG_RENDER_MODE_XRAY;

	DebugDrawBox(OBB3(chunk->GetBoundsWs()), options);
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
	std::map<IntVector3, Chunk*>::iterator itr = m_activeChunks.begin();
	for (itr; itr != m_activeChunks.end(); itr++)
	{
		Chunk* chunk = itr->second;
		chunk->Update(deltaSeconds);
	}
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

	// TODO: Hook up the references to the neighbors
	/*IntVector2 eastCoords = chunkCoords + IntVector2(1, 0);
	IntVector2 westCoords = chunkCoords + IntVector2(-1, 0);
	IntVector2 northCoords = chunkCoords + IntVector2(0, 1);
	IntVector2 southCoords = chunkCoords + IntVector2(0, -1);

	bool eastExists = m_activeChunks.find(eastCoords) != m_activeChunks.end();
	bool westExists = m_activeChunks.find(westCoords) != m_activeChunks.end();
	bool northExists = m_activeChunks.find(northCoords) != m_activeChunks.end();
	bool southExists = m_activeChunks.find(southCoords) != m_activeChunks.end();

	if (eastExists)
	{
		Chunk* eastChunk = m_activeChunks[eastCoords];

		chunkToAdd->SetEastNeighbor(eastChunk);
		eastChunk->SetWestNeighbor(chunkToAdd);
	}

	if (westExists)
	{
		Chunk* westChunk = m_activeChunks[westCoords];

		chunkToAdd->SetWestNeighbor(westChunk);
		westChunk->SetEastNeighbor(chunkToAdd);
	}

	if (northExists)
	{
		Chunk* northChunk = m_activeChunks[northCoords];

		chunkToAdd->SetNorthNeighbor(northChunk);
		northChunk->SetSouthNeighbor(chunkToAdd);
	}

	if (southExists)
	{
		Chunk* southChunk = m_activeChunks[southCoords];

		chunkToAdd->SetSouthNeighbor(southChunk);
		southChunk->SetNorthNeighbor(chunkToAdd);
	}*/
}
