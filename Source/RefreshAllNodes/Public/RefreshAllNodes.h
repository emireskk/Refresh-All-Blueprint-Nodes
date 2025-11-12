#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FRefreshAllNodesModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void RefreshAllBlueprintNodes();
	void RefreshBlueprintsInFolder();

private:
	void RegisterMenus();
	void RegisterContentBrowserContextMenu();
	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedPtr<class FUICommandList> PluginCommands;
	TArray<FString> SelectedPaths;
};