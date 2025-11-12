#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "RefreshAllNodesStyle.h"

class FRefreshAllNodesCommands : public TCommands<FRefreshAllNodesCommands>
{
public:
	FRefreshAllNodesCommands()
		: TCommands<FRefreshAllNodesCommands>(TEXT("RefreshAllNodes"),
			NSLOCTEXT("Contexts", "RefreshAllNodes", "Refresh All Nodes Plugin"),
			NAME_None,
			FRefreshAllNodesStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> RefreshAction;
};