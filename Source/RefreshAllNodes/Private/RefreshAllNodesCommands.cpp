#include "RefreshAllNodesCommands.h"

#define LOCTEXT_NAMESPACE "FRefreshAllNodesModule"

void FRefreshAllNodesCommands::RegisterCommands()
{
	UI_COMMAND(RefreshAction, "Refresh All Blueprint Nodes", "Refresh all nodes in all Blueprints in the project", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE