///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: November 10th, 2020
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Framework/App.h"
#include "Game/Framework/Game.h"
#include "Game/Framework/GameCommands.h"
#include "Game/Framework/GameCommon.h"
#include "Engine/Collision/CollisionScene.h"
#include "Engine/Collision/BoundingVolumeHierarchy/BoundingVolume.h"
#include "Engine/Core/DevConsole.h"
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

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS IMPLEMENTATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void Command_Exit(CommandArgs& args)
{
	UNUSED(args);
	g_app->Quit();
	ConsoleLogf("Exiting...");
}


//-------------------------------------------------------------------------------------------------
void Command_SetCollisionDebugFlags(CommandArgs& args)
{
	CollisionDebugFlags flags = 0;

	std::string flagName = args.GetNextString(false);
	while (flagName.size() > 0)
	{
		if (AreEqualCaseInsensitive(flagName, "collider"))
		{
			flags |= COLLISION_DEBUG_COLLIDERS;
			ConsoleLogf("Enabled collider debug");
		}
		else if (AreEqualCaseInsensitive(flagName, "contact"))
		{
			flags |= COLLISION_DEBUG_CONTACTS;
			ConsoleLogf("Enabled contact debug");
		}
		else if (AreEqualCaseInsensitive(flagName, "bounds"))
		{
			flags |= COLLISION_DEBUG_BOUNDING_VOLUMES;
			ConsoleLogf("Enabled bounding volume hierarchy debug");
		}
		else if (AreEqualCaseInsensitive(flagName, "leaf_bounds"))
		{
			flags |= COLLISION_DEBUG_LEAF_BOUNDING_VOLUMES;
			ConsoleLogf("Enabled leaf bounding volume debug");
		}
		else
		{
			ConsoleLogErrorf("Invalid collision flag name: %s\n", flagName.c_str());
		}

		flagName = args.GetNextString(false);
	}


	CollisionScene<BoundingVolumeSphere>* scene = g_game->GetCollisionScene();
	scene->SetDebugFlags(flags);
}
