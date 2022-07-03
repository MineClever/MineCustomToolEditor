#include "AssetMenuTools/FCommonAssetActionsListener.h"
#include "PackageTools.h"
#include "AssetMenuTools/FAssetsProcessorFormSelection.hpp"



#define LOCTEXT_NAMESPACE "FCommanAssetActionsListener"

/* Commands */
class MineAssetCtxMenuCommands : public TCommands<MineAssetCtxMenuCommands>
{
public:

    /* INIT */
    MineAssetCtxMenuCommands ()
        : TCommands<MineAssetCtxMenuCommands> (
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

        UI_COMMAND (MenuCommand1,
            "Reload selected assets",
            "Reload selected assets form top package.",
            EUserInterfaceActionType::Button, FInputGesture ()
        );

    }

public:
    /* Command Action Objects */
    TSharedPtr<FUICommandInfo> MenuCommand1;

};


/* Processor */
class FCommonAssetReloadPackagesProcessor : public FAssetsProcessorFormSelection_Base
{

    virtual void Execute () override
    {
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Reload all selected assets without save"));
        TArray<UPackage *> AssetPackages;

        for (auto const AssetData: SelectedAssets)
        {
            UPackage* PackageName = AssetData.GetPackage();
            AssetPackages.AddUnique (PackageName);
        }
        UPackageTools::ReloadPackages (AssetPackages);
    }
};


/* Extension to menu */
class FMineContentBrowserExtensions_CommonAssets
{
public:


    static void ExecuteProcessor (
        const TSharedPtr<FAssetsProcessorFormSelection_Base> Processor
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

        if (!CommandList.IsValid())
            CommandList = MakeShareable (new FUICommandList);
        TSharedRef<FExtender> Extender (new FExtender ());

        MappingCommand (CommandList,SelectedAssets);

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
        static const MineAssetCtxMenuCommands &ToolCommands = MineAssetCtxMenuCommands::Get ();
        MenuBuilder.AddMenuEntry (ToolCommands.MenuCommand1);
    }


    template<typename P>
    static TSharedPtr<FAssetsProcessorFormSelection_Base> &
    CreateProcessorPtr (const TArray<FAssetData> &SelectedAssets)
    {
        static_assert (
            std::is_base_of_v<FAssetsProcessorFormSelection_Base, P>,
            "Must be derived from FAssetsProcessorFormSelection_Base"
        );

        TSharedPtr<P> const Processor = MakeShareable (new P);//On Heap
        Processor->SelectedAssets = SelectedAssets;
        return StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (Processor);
    }


    static void MappingCommand (
        const TSharedPtr<FUICommandList> &CommandList,
        const TArray<FAssetData> &SelectedAssets
    )
    {
        static const MineAssetCtxMenuCommands &ToolCommands = MineAssetCtxMenuCommands::Get ();
        CommandList->MapAction (
            ToolCommands.MenuCommand1,
            FExecuteAction::CreateStatic(
                &ExecuteProcessor,
                CreateProcessorPtr<FCommonAssetReloadPackagesProcessor> (SelectedAssets)
            ),
            FCanExecuteAction()
        );
    }
};


/* Load to module */

void FCommonAssetActionsListener::InstallHooks()
{
    UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install Common Asset Menu Hook"));
    // register commands
    MineAssetCtxMenuCommands::Register ();

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