///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: Jan 17th, 2022
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Block/BlockDefinition.h"
#include "Engine/Core/EngineCommon.h"

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------
uint8 BlockDefinition::s_numDefs = 0;
std::map<StringID, uint8> BlockDefinition::s_defNames;
BlockDefinition BlockDefinition::s_definitions[MAX_BLOCK_DEFS];

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS IMPLEMENTATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void BlockDefinition::InitializeBuiltInDefs()
{
	// Air
	{
		BlockDefinition airDef;
		airDef.m_id = SID("air");
		airDef.m_color = Rgba(0, 0, 0, 0);
		AddDef(airDef);
	}
	
	// Grass
	{
		BlockDefinition grassDef;
		grassDef.m_id = SID("grass");
		grassDef.m_color = Rgba::GREEN;
		AddDef(grassDef);
	}


	// Dirt
	{
		BlockDefinition dirtDef;
		dirtDef.m_id = SID("dirt");
		dirtDef.m_color = Rgba(76, 54, 17, 255);
		AddDef(dirtDef);
	}

	// Stone
	{
		BlockDefinition stoneDef;
		stoneDef.m_id = SID("stone");
		stoneDef.m_color = Rgba::GRAY;
		AddDef(stoneDef);
	}

	// Water
	{
		BlockDefinition waterDef;
		waterDef.m_id = SID("water");
		waterDef.m_color = Rgba::BLUE;
		AddDef(waterDef);
	}
}


//-------------------------------------------------------------------------------------------------
const BlockDefinition* BlockDefinition::GetDefinition(const StringID& id)
{
	std::map<StringID, uint8>::const_iterator itr = s_defNames.find(id);
	ASSERT_OR_DIE(itr != s_defNames.end(), "Definition not found!");

	return GetDefinition(itr->second);
}


//-------------------------------------------------------------------------------------------------
const BlockDefinition* BlockDefinition::GetDefinition(uint8 index)
{
	ASSERT_OR_DIE(index < s_numDefs, "Invalid index!");
	return &s_definitions[index];
}


//-------------------------------------------------------------------------------------------------
void BlockDefinition::AddDef(BlockDefinition& def)
{
	// Set index on the def
	def.m_index = s_numDefs;

	// Add to the registries
	s_defNames[def.m_id] = def.m_index;
	s_definitions[def.m_index] = def;

	// Increment number of defs
	s_numDefs++;
}