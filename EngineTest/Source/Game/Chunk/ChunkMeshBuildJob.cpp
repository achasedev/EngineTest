///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: Feb 3rd, 2022
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Chunk/Chunk.h"
#include "Game/Chunk/ChunkMeshBuilder.h"
#include "Game/Chunk/ChunkMeshBuildJob.h"
#include "Game/Framework/World.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Render/RenderScene.h"
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
ChunkMeshBuildJob::ChunkMeshBuildJob(Chunk* chunk, ChunkMeshType meshType, World* world)
	: Job(false)
	, m_chunk(chunk)
	, m_meshType(meshType)
	, m_world(world)
{
	m_jobType = JOB_TYPE_INDEX;
}


//-------------------------------------------------------------------------------------------------
ChunkMeshBuildJob::~ChunkMeshBuildJob()
{
}


//-------------------------------------------------------------------------------------------------
void ChunkMeshBuildJob::Execute()
{
	m_meshBuilt = m_meshBuilder.BuildMeshForChunk(m_chunk, m_meshType);
}


//-------------------------------------------------------------------------------------------------
void ChunkMeshBuildJob::Finalize()
{
	RenderScene* scene = m_world->GetRenderScene();

	if (!m_meshBuilt)
	{
		// If the mesh wasn't built now, remove it from the render scene
		Renderable* rend = scene->GetRenderable(m_chunk->GetRenderSceneId());

		if (rend != nullptr)
		{
			scene->RemoveRenderable(m_chunk->GetRenderSceneId());
		}
	}
	else if (m_chunk->GetRenderSceneId() == INVALID_RENDER_SCENE_ID)
	{
		// Update the GPU
		Mesh* chunkMesh = m_chunk->CreateOrGetMesh();
		m_meshBuilder.UpdateMesh(chunkMesh);

		// Mesh was built - if it wasn't already in the render scene, add it
		Renderable rend;
		Material* material = g_resourceSystem->CreateOrGetMaterial("Data/Material/chunk.material");
		rend.AddDraw(m_chunk->GetMesh(), material);

		RenderSceneId chunkSceneId = scene->AddRenderable(rend);
		m_chunk->SetRenderSceneId(chunkSceneId);
	}
}
