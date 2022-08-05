#include "ObjectTools.h"
#include "AssetMenuTools/FCommonAssetActionsListener.h"
#include "PackageTools.h"
#include "AssetMenuTools/TAssetsProcessorFormSelection.hpp"

#define LOCTEXT_NAMESPACE "FCommanAssetActionsListener"

/* Commands */
namespace FCommonAssetActionsMenuCommandsInfo_Internal
{
    class MineAssetCtxMenuCommandsInfo : public TCommands<MineAssetCtxMenuCommandsInfo>
    {
    public:

        /* INIT */
        MineAssetCtxMenuCommandsInfo ()
            : TCommands<MineAssetCtxMenuCommandsInfo> (
                TEXT ("MineCommonAssetCtxMenu"), // Context name for fast lookup
                FText::FromString ("MineCommonAssetCtxMenu"), // Context name for displaying
                NAME_None,   // No parent context
                FEditorStyle::GetStyleSetName () // Icon Style Set
                )
        {
        }

        /* Register all commands in this class */
        virtual void RegisterCommands () override
        {

            UI_COMMAND (MenuCommandInfo_0,
                "Reload selected assets",
                "Reload selected assets form top package.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );
            UI_COMMAND (MenuCommandInfo_1,
                "Copy Asset Content Path",
                "Copy Asset Content Path from selected.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );
            UI_COMMAND (MenuCommandInfo_2,
                "Make Read-only",
                "Make Selected assets Read-only",
                EUserInterfaceActionType::Button, FInputGesture ()
            );
            UI_COMMAND (MenuCommandInfo_3,
                "Make Writable",
                "Make Selected assets Writable",
                EUserInterfaceActionType::Button, FInputGesture ()
            );
        }

    public:
        /* Command Action Objects */
        TSharedPtr<FUICommandInfo> MenuCommandInfo_0;
        TSharedPtr<FUICommandInfo> MenuCommandInfo_1;
        TSharedPtr<FUICommandInfo> MenuCommandInfo_2;
        TSharedPtr<FUICommandInfo> MenuCommandInfo_3;
    };

};

/* Processor */
namespace FCommonAssetActionProcessors_Internal
{
    class FCommonAssetReloadPackagesProcessor : public FAssetsProcessorFormSelection_Base
    {

        virtual void Execute () override
        {
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Reload all selected assets without save"));
            TArray<UPackage *> AssetPackages;

            for (auto const AssetData : SelectedAssets) {
                UPackage *PackageName = AssetData.GetPackage ();
                AssetPackages.AddUnique (PackageName);
            }

            UPackageTools::ReloadPackages (AssetPackages);
        }
    };

    // Read-Only Fix?
    class FCommonAssetReadOnlyProcessor : public FAssetsProcessorFormSelection_Base
    {

        virtual void Execute () override
        {

            UAssetEditorSubsystem *&&AssetSubSystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();
            TArray<FString> FilesPath;
            for (auto const AssetData : SelectedAssets) {
                // Make sure asset unload!
                auto const Asset = AssetData.GetAsset ();
                AssetSubSystem->CloseAllEditorsForAsset (Asset);
                FString AssetPathName = Asset->GetPackage ()->GetPathName ();
                FAssetSourceControlHelper::GetLowLevel ().SetReadOnly (*SourceControlHelpersInternal::ConvertFileToQualifiedPath(AssetPathName,true,false), true);
                FilesPath.Add (AssetPathName);
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Mark Asset Read-only : %s"), *AssetPathName);
            }
        }
    };

    class FCommonAssetWritableProcessor : public FAssetsProcessorFormSelection_Base
    {

        virtual void Execute () override
        {

            UAssetEditorSubsystem *&&AssetSubSystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();
            TArray<FString> FilesPath;
            for (auto const AssetData : SelectedAssets) {
                // Make sure asset unload!
                auto const Asset = AssetData.GetAsset ();
                AssetSubSystem->CloseAllEditorsForAsset (Asset);
                FString AssetPathName = Asset->GetPackage ()->GetPathName ();
                FAssetSourceControlHelper::GetLowLevel ().SetReadOnly (*SourceControlHelpersInternal::ConvertFileToQualifiedPath (AssetPathName, true, false), false);
                FilesPath.Add (AssetPathName);
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Mark Asset Writable : %s"), *AssetPathName);
            }
        }
    };

    class FCommonAssetCopyPackagesPathProcessor : public FAssetsProcessorFormSelection_Base
    {

        virtual void Execute () override
        {
            FString StringArrayToCopy = "";
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Static Mesh Copy Assets Path Processor Run !! "));
            uint32 LoopCount = 1;
            for (auto const Asset : SelectedAssets) {
                const FString TempAssetPathName = Asset.GetPackage ()->GetPathName ();
                UE_LOG (LogMineCustomToolEditor, Log, TEXT ("%d : %s"), LoopCount, *(TempAssetPathName));
                GEngine->AddOnScreenDebugMessage (-1, 5.f, FColor::Blue, *(TempAssetPathName));
                StringArrayToCopy.Append (TempAssetPathName);
                StringArrayToCopy.Append ("\n");
                ++LoopCount;
            }
            FPlatformMisc::ClipboardCopy (*StringArrayToCopy);
        }
    };

}


/* Extension to menu */
namespace FCommonAssetContentBrowserExtensions_Internal
{
    using namespace FCommonAssetActionsMenuCommandsInfo_Internal;
    using namespace FCommonAssetActionProcessors_Internal;

    class FMineContentBrowserExtensions_CommonAssets
    {
    public:

        static void ExecuteProcessor (
            TSharedPtr<FAssetsProcessorFormSelection_Base> const Processor
        )
        {
            Processor->Execute ();
        }

        static TSharedRef<FExtender>
            OnExtendContentBrowserAssetSelectionMenu (
                const TArray<FAssetData> &SelectedAssets
            )
        {
            static TSharedPtr<FUICommandList> CommandList;

            if (!CommandList.IsValid ())
                CommandList = MakeShareable (new FUICommandList);
            TSharedRef<FExtender> Extender (new FExtender ());

            MappingCommand (CommandList, SelectedAssets);

            if (SelectedAssets.Num () > 0) {
                // Add the Static actions sub-menu extender
                Extender->AddMenuExtension (
                    "GetAssetActions",
                    EExtensionHook::After,
                    CommandList,
                    FMenuExtensionDelegate::CreateStatic (
                        &CreateActionsSubMenu)
                );
            }
            return Extender;
        };

        static void CreateActionsSubMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            const FSlateIcon BaseMenuIcon = FSlateIcon ();
            const FText BaseMenuName = LOCTEXT ("ActionsSubMenuLabel", "Mine Common Asset Actions");
            const FText BaseMenuTip = LOCTEXT ("ActionsSubMenuToolTip", "Type-related actions for Common Asset.");

            MenuBuilder.AddSubMenu (
                BaseMenuName,
                BaseMenuTip,
                FNewMenuDelegate::CreateStatic (
                    &PopulateActionsMenu),
                false,
                BaseMenuIcon,
                true,
                FName (TEXT ("MineAssetsActions"))
            );

        }

        static void PopulateActionsMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            // Add to Menu
            static const MineAssetCtxMenuCommandsInfo &ToolCommandsInfo = MineAssetCtxMenuCommandsInfo::Get ();
            MenuBuilder.AddMenuEntry (ToolCommandsInfo.MenuCommandInfo_0);
            MenuBuilder.AddMenuEntry (ToolCommandsInfo.MenuCommandInfo_1);
            MenuBuilder.AddMenuEntry (ToolCommandsInfo.MenuCommandInfo_2);
            MenuBuilder.AddMenuEntry (ToolCommandsInfo.MenuCommandInfo_3);
        }


        static void MappingCommand (
            const TSharedPtr<FUICommandList> &CommandList,
            const TArray<FAssetData> &SelectedAssets
        )
        {
            static const MineAssetCtxMenuCommandsInfo &ToolCommandsInfo = MineAssetCtxMenuCommandsInfo::Get ();
            CommandList->MapAction (
                ToolCommandsInfo.MenuCommandInfo_0,
                FExecuteAction::CreateStatic (
                    &ExecuteProcessor,
                    AssetsProcessorCastHelper::CreateBaseProcessorPtr<FCommonAssetReloadPackagesProcessor> (SelectedAssets)
                ),
                FCanExecuteAction ()
            );
            CommandList->MapAction (
                ToolCommandsInfo.MenuCommandInfo_1,
                FExecuteAction::CreateStatic (
                    &ExecuteProcessor,
                    AssetsProcessorCastHelper::CreateBaseProcessorPtr<FCommonAssetCopyPackagesPathProcessor> (SelectedAssets)
                ),
                FCanExecuteAction ()
            );
            CommandList->MapAction (
                ToolCommandsInfo.MenuCommandInfo_2,
                FExecuteAction::CreateStatic (
                    &ExecuteProcessor,
                    AssetsProcessorCastHelper::CreateBaseProcessorPtr<FCommonAssetReadOnlyProcessor> (SelectedAssets)
                ),
                FCanExecuteAction ()
            );
            CommandList->MapAction (
                ToolCommandsInfo.MenuCommandInfo_3,
                FExecuteAction::CreateStatic (
                    &ExecuteProcessor,
                    AssetsProcessorCastHelper::CreateBaseProcessorPtr<FCommonAssetWritableProcessor> (SelectedAssets)
                ),
                FCanExecuteAction ()
            );

        }
    };

}


/* Load to module */

void FCommonAssetActionsListener::InstallHooks()
{
    using namespace FCommonAssetContentBrowserExtensions_Internal;
    UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install Common Asset Menu Hook"));
    // register commands
    MineAssetCtxMenuCommandsInfo::Register ();

    // Declare Delegate 
    ContentBrowserExtenderDelegate =
        FContentBrowserMenuExtender_SelectedAssets::CreateStatic (
            &FMineContentBrowserExtensions_CommonAssets::OnExtendContentBrowserAssetSelectionMenu);

    // Get all content module delegates
    TArray<FContentBrowserMenuExtender_SelectedAssets>
        &CBMenuExtenderDelegates = GetExtenderDelegates ();

    // Add The delegate of mine
    CBMenuExtenderDelegates.Add (ContentBrowserExtenderDelegate);
    ContentBrowserExtenderDelegateHandle =
        CBMenuExtenderDelegates.Last ().GetHandle ();

}

void FCommonAssetActionsListener::RemoveHooks()
{
    TArray<FContentBrowserMenuExtender_SelectedAssets>
        &CBMenuExtenderDelegates = GetExtenderDelegates ();

    CBMenuExtenderDelegates.RemoveAll (
        [&](const FContentBrowserMenuExtender_SelectedAssets &Delegate)->bool {
            return Delegate.GetHandle () == ContentBrowserExtenderDelegateHandle;
        }
    );
}

TArray<FContentBrowserMenuExtender_SelectedAssets> &
FCommonAssetActionsListener::GetExtenderDelegates ()
{
    /////////////////////////////
    ///Get ContentBrowser Module
    /////////////////////////////
    FContentBrowserModule &ContentBrowserModule =
        FModuleManager::LoadModuleChecked<FContentBrowserModule> (TEXT ("ContentBrowser"));

    return ContentBrowserModule.GetAllAssetViewContextMenuExtenders ();
}

#undef LOCTEXT_NAMESPACE