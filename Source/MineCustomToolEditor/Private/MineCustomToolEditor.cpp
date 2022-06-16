#include "MineCustomToolEditor.h"
#include "MenuTools/MenuTool.h"
#include "TabTools/TabTool.h"


IMPLEMENT_GAME_MODULE(FMineToolEditor, MineCustomToolEditor)

#define LOCTEXT_NAMESPACE "MineCustomToolEditor"

//Define General Log Category
DEFINE_LOG_CATEGORY (LogMineCustomToolEditor);

/* Start?! */

void FMineToolEditor::AddModuleListeners ()
{
    // Add Custom Command
    ModuleListeners.Emplace (MakeShareable(new MenuTool));
    // Add Custom Panel
    ModuleListeners.Emplace (MakeShareable (new TabTool));
}

void FMineToolEditor::StartupModule ()
{

    if (!IsRunningCommandlet ()) {
        /* 1. Find LevelEditor */
        FLevelEditorModule &LevelEditorModule = 
            FModuleManager::LoadModuleChecked<FLevelEditorModule> ("LevelEditor");

        /* 2. Find Menu Extension Manager*/
        LevelEditorMenuExtensibilityManager = LevelEditorModule.GetMenuExtensibilityManager ();

        /* 3. Make MenuExtender */
        MenuExtender = MakeShareable (new FExtender);
        // Make MenuExtender Builder Callback
        auto &&MenuBarExtensionDelegate = 
            FMenuBarExtensionDelegate::CreateRaw (
                this, &FMineToolEditor::MakePulldownMenu
            );
        // Init MenuExtender Object
        MenuExtender->AddMenuBarExtension (
            "Window", EExtensionHook::After,
            nullptr,
            MenuBarExtensionDelegate);

        /* 4. Add Extender to Extension Manager */
        LevelEditorMenuExtensibilityManager->AddExtender (MenuExtender);


    }

#ifdef MINE_EDITOR_LOAD_EXAMPLE_ACTOR
    // register custom layouts
    {
        static FName PropertyEditor ("PropertyEditor");
        FPropertyEditorModule &PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule> (PropertyEditor);

        PropertyModule.RegisterCustomClassLayout (
            AExampleActor::StaticClass ()->GetFName (),
            FOnGetDetailCustomizationInstance::CreateStatic (&FExampleActorDetails::MakeInstance)
        );
    }
#endif

#ifdef MINE_EDITOR_LOAD_EXAMPLE_CUSTOM_DATA
    // register custom types:
    {
        IAssetTools &AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule> ("AssetTools").Get ();
        // add custom category
        EAssetTypeCategories::Type ExampleCategory = 
            AssetTools.RegisterAdvancedAssetCategory (
                FName (TEXT ("Example")),
                FText::FromString ("Example")
            );
        // register our custom asset with example category
        TSharedPtr<IAssetTypeActions> Action = MakeShareable (new FExampleDataTypeActions (ExampleCategory));
        AssetTools.RegisterAssetTypeActions (Action.ToSharedRef ());
        // saved it here for unregister later
        CreatedAssetTypeActions.Add (Action);
    }
#endif

    /* Start Listener */
    IMineCustomToolModuleInterface::StartupModule ();
}

void FMineToolEditor::ShutdownModule ()
{

#ifdef MINE_EDITOR_LOAD_EXAMPLE_ACTOR
    // unregister custom layouts
    if (FModuleManager::Get ().IsModuleLoaded ("PropertyEditor")) {
        FPropertyEditorModule &PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule> ("PropertyEditor");

        PropertyModule.UnregisterCustomClassLayout (
            AExampleActor::StaticClass ()->GetFName ()
        );
    }
#endif

#ifdef MINE_EDITOR_LOAD_EXAMPLE_CUSTOM_DATA
    // Unregister all the asset types that we registered
    if (FModuleManager::Get ().IsModuleLoaded ("AssetTools")) {
        IAssetTools &AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule> ("AssetTools").Get ();
        for (int32 i = 0; i < CreatedAssetTypeActions.Num (); ++i) {
            AssetTools.UnregisterAssetTypeActions (CreatedAssetTypeActions[i].ToSharedRef ());
        }
    }
    CreatedAssetTypeActions.Empty ();
#endif

    IMineCustomToolModuleInterface::ShutdownModule ();
}


/* Main */

TSharedRef<FWorkspaceItem> FMineToolEditor::MenuRoot = 
    FWorkspaceItem::NewGroup (FText::FromString ("Mine Menu Root"));


void FMineToolEditor::AddMenuExtension (
    const FMenuExtensionDelegate &extensionDelegate,
    FName extensionHook,
    const TSharedPtr<FUICommandList> &CommandList,
    EExtensionHook::Position position )
{
    MenuExtender->AddMenuExtension (extensionHook, position, CommandList, extensionDelegate);
}

void FMineToolEditor::MakePulldownMenu (FMenuBarBuilder &menuBuilder)
{
    auto MenuDelegate = FNewMenuDelegate::CreateRaw (
        this, &FMineToolEditor::FillPulldownMenu
    );

    menuBuilder.AddPullDownMenu (
        FText::FromString ("MineToolMenu"),
        FText::FromString ("Open the MineToolMenu To Extended Button"),
        MenuDelegate,
        "MineToolMenu",
        FName (TEXT ("MineToolMenu"))
    );
}

void FMineToolEditor::FillPulldownMenu (FMenuBuilder &menuBuilder)
{
    // just a frame for tools to fill in Section_1
    
    menuBuilder.BeginSection ("MineToolMenu", FText::FromString ("Mine Cpp Menu"));
    menuBuilder.AddMenuSeparator (FName ("Section_1"));
    menuBuilder.EndSection ();

    // just a frame for tools to fill in Section_2
    menuBuilder.BeginSection ("MineToolMenu", FText::FromString ("Mine Tab Menu"));
    menuBuilder.AddMenuSeparator (FName ("Section_2"));
    menuBuilder.EndSection ();

    FText TempConsoleString = LOCTEXT ("LogUserOutMenu", "Pop From MineToolMenu");
    GEngine->AddOnScreenDebugMessage (-1, 2.0f, FColor::Blue, TempConsoleString.ToString());

}

#undef LOCTEXT_NAMESPACE