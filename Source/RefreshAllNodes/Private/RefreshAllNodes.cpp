#include "RefreshAllNodes.h"
#include "RefreshAllNodesStyle.h"
#include "RefreshAllNodesCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

static const FName RefreshAllNodesTabName("RefreshAllNodes");

#define LOCTEXT_NAMESPACE "FRefreshAllNodesModule"

void FRefreshAllNodesModule::StartupModule()
{
	FRefreshAllNodesStyle::Initialize();
	FRefreshAllNodesStyle::ReloadTextures();

	FRefreshAllNodesCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FRefreshAllNodesCommands::Get().RefreshAction,
		FExecuteAction::CreateRaw(this, &FRefreshAllNodesModule::RefreshAllBlueprintNodes),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FRefreshAllNodesModule::RegisterMenus));

	RegisterContentBrowserContextMenu();
}

void FRefreshAllNodesModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FRefreshAllNodesStyle::Shutdown();
	FRefreshAllNodesCommands::Unregister();
}

void FRefreshAllNodesModule::RefreshAllBlueprintNodes()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> BlueprintAssets;
	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	AssetRegistry.GetAssets(Filter, BlueprintAssets);

	// Filter Blueprints in the Engine folder - only take those under /Game/
	TArray<FAssetData> FilteredBlueprints;
	for (const FAssetData& AssetData : BlueprintAssets)
	{
		FString PackagePath = AssetData.PackageName.ToString();
		if (PackagePath.StartsWith(TEXT("/Game/")))
		{
			FilteredBlueprints.Add(AssetData);
		}
	}

	if (FilteredBlueprints.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprints", "No Blueprints found in the Content folder."));
		return;
	}

	EAppReturnType::Type Result = FMessageDialog::Open(
		EAppMsgType::YesNo,
		FText::Format(LOCTEXT("ConfirmRefresh", "Found {0} Blueprint(s) in Content folder. This operation may take a while. Continue?"),
			FText::AsNumber(FilteredBlueprints.Num()))
	);

	if (Result != EAppReturnType::Yes)
	{
		return;
	}

	int32 ProcessedCount = 0;
	int32 ErrorCount = 0;

	FScopedSlowTask SlowTask(FilteredBlueprints.Num(), LOCTEXT("RefreshingBlueprints", "Refreshing Blueprint Nodes..."));
	SlowTask.MakeDialog(true);

	for (const FAssetData& AssetData : FilteredBlueprints)
	{
		if (SlowTask.ShouldCancel())
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("RefreshCancelled", "Operation cancelled by user.\nProcessed: {0}\nRemaining: {1}"),
					FText::AsNumber(ProcessedCount),
					FText::AsNumber(FilteredBlueprints.Num() - ProcessedCount)));
			return;
		}

		SlowTask.EnterProgressFrame(1, FText::Format(LOCTEXT("ProcessingBlueprint", "Processing: {0}"), FText::FromName(AssetData.AssetName)));

		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (Blueprint)
		{
			try
			{
				// Refresh all nodes in all graphs
				TArray<UEdGraph*> Graphs;
				Blueprint->GetAllGraphs(Graphs);

				for (UEdGraph* Graph : Graphs)
				{
					if (Graph)
					{
						for (UEdGraphNode* Node : Graph->Nodes)
						{
							if (Node)
							{
								// Reconstruct node
								Node->ReconstructNode();
							}
						}
					}
				}

				// Mark package as dirty and save
				Blueprint->MarkPackageDirty();
				FBlueprintEditorUtils::RefreshAllNodes(Blueprint);

				ProcessedCount++;
			}
			catch (...)
			{
				ErrorCount++;
				UE_LOG(LogTemp, Warning, TEXT("Failed to refresh Blueprint: %s"), *AssetData.AssetName.ToString());
			}
		}
	}

	FText ResultMessage = FText::Format(
		LOCTEXT("RefreshComplete", "Refresh complete!\nProcessed: {0}\nErrors: {1}"),
		FText::AsNumber(ProcessedCount),
		FText::AsNumber(ErrorCount)
	);

	FMessageDialog::Open(EAppMsgType::Ok, ResultMessage);
}

void FRefreshAllNodesModule::RefreshBlueprintsInFolder()
{
	if (SelectedPaths.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoFolderSelected", "No folder selected."));
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> BlueprintAssets;

	for (const FString& Path : SelectedPaths)
	{
		// Engine klasörlerini engelle
		if (Path.StartsWith(TEXT("/Engine/")))
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				LOCTEXT("EnginePathBlocked", "Cannot refresh Blueprints in Engine folders. Only Content folder (/Game/) is allowed."));
			return;
		}

		FARFilter Filter;
		Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(FName(*Path));

		TArray<FAssetData> PathAssets;
		AssetRegistry.GetAssets(Filter, PathAssets);
		BlueprintAssets.Append(PathAssets);
	}

	if (BlueprintAssets.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprintsInFolder", "No Blueprints found in selected folder(s)."));
		return;
	}

	FString FolderNames;
	for (int32 i = 0; i < SelectedPaths.Num(); i++)
	{
		FolderNames += SelectedPaths[i];
		if (i < SelectedPaths.Num() - 1)
		{
			FolderNames += TEXT(", ");
		}
	}

	EAppReturnType::Type Result = FMessageDialog::Open(
		EAppMsgType::YesNo,
		FText::Format(LOCTEXT("ConfirmFolderRefresh", "Found {0} Blueprint(s) in selected folder(s):\n{1}\n\nThis operation may take a while. Continue?"),
			FText::AsNumber(BlueprintAssets.Num()),
			FText::FromString(FolderNames))
	);

	if (Result != EAppReturnType::Yes)
	{
		return;
	}

	int32 ProcessedCount = 0;
	int32 ErrorCount = 0;

	FScopedSlowTask SlowTask(BlueprintAssets.Num(), LOCTEXT("RefreshingFolderBlueprints", "Refreshing Blueprint Nodes in Folder..."));
	SlowTask.MakeDialog(true);

	for (const FAssetData& AssetData : BlueprintAssets)
	{
		if (SlowTask.ShouldCancel())
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("FolderRefreshCancelled", "Operation cancelled by user.\nProcessed: {0}\nRemaining: {1}"),
					FText::AsNumber(ProcessedCount),
					FText::AsNumber(BlueprintAssets.Num() - ProcessedCount)));
			return;
		}

		SlowTask.EnterProgressFrame(1, FText::Format(LOCTEXT("ProcessingBlueprint", "Processing: {0}"), FText::FromName(AssetData.AssetName)));

		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (Blueprint)
		{
			try
			{
				TArray<UEdGraph*> Graphs;
				Blueprint->GetAllGraphs(Graphs);

				for (UEdGraph* Graph : Graphs)
				{
					if (Graph)
					{
						for (UEdGraphNode* Node : Graph->Nodes)
						{
							if (Node)
							{
								Node->ReconstructNode();
							}
						}
					}
				}

				Blueprint->MarkPackageDirty();
				FBlueprintEditorUtils::RefreshAllNodes(Blueprint);

				ProcessedCount++;
			}
			catch (...)
			{
				ErrorCount++;
				UE_LOG(LogTemp, Warning, TEXT("Failed to refresh Blueprint: %s"), *AssetData.AssetName.ToString());
			}
		}
	}

	FText ResultMessage = FText::Format(
		LOCTEXT("FolderRefreshComplete", "Folder refresh complete!\nProcessed: {0}\nErrors: {1}"),
		FText::AsNumber(ProcessedCount),
		FText::AsNumber(ErrorCount)
	);

	FMessageDialog::Open(EAppMsgType::Ok, ResultMessage);
}

void FRefreshAllNodesModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("CustomTools");
			Section.AddMenuEntryWithCommandList(FRefreshAllNodesCommands::Get().RefreshAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FRefreshAllNodesCommands::Get().RefreshAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

void FRefreshAllNodesModule::RegisterContentBrowserContextMenu()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedPaths>& MenuExtenders = ContentBrowserModule.GetAllPathViewContextMenuExtenders();

	MenuExtenders.Add(FContentBrowserMenuExtender_SelectedPaths::CreateLambda(
		[this](const TArray<FString>& SelectedPaths)
		{
			TSharedRef<FExtender> Extender = MakeShared<FExtender>();

			this->SelectedPaths = SelectedPaths;

			Extender->AddMenuExtension(
				"PathContextBulkOperations",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
					{
						MenuBuilder.AddMenuEntry(
							LOCTEXT("RefreshBlueprintsInFolder", "Refresh Blueprint Nodes in Folder"),
							LOCTEXT("RefreshBlueprintsInFolderTooltip", "Refresh all Blueprint nodes in this folder and subfolders"),
							FSlateIcon(FRefreshAllNodesStyle::GetStyleSetName(), "RefreshAllNodes.RefreshAction"),
							FUIAction(FExecuteAction::CreateRaw(this, &FRefreshAllNodesModule::RefreshBlueprintsInFolder))
						);
					})
			);

			return Extender;
		}
	));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRefreshAllNodesModule, RefreshAllNodes)