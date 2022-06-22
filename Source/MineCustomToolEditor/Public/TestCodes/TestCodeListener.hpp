#pragma once
#include "Interfaces/IMineCustomToolModuleInterface.h"
#include "CoreMinimal.h"
#include "MineMouduleDefine.h"
#include "Layers/Public/LayersModule.h"
#include "Layers/Private/LayerCollectionViewModel.h"
#include "Layers/Private/LayerCollectionViewCommands.h"



#define LOCTEXT_NAMESPACE "TestCodeListener"

class LayerMenuToolCommands : public TCommands<LayerMenuToolCommands>
{
public:

    /* INIT */
    LayerMenuToolCommands ()
        : TCommands<LayerMenuToolCommands> (
            TEXT ("MineLayerTool"), // Context name for fast lookup
            LOCTEXT ("MineLayerToolMenu", "Mine Custom Layer Menu"), // Context name for displaying
            NAME_None,   // No parent context
            FEditorStyle::GetStyleSetName () // Icon Style Set
            )
    {
    }

    /* Register all commands in this class */
    virtual void RegisterCommands () override
    {
        UI_COMMAND (MenuCommand1,
            "Expor Layers Name", "Export All Layer Name in Layer Content.",
            EUserInterfaceActionType::Button,
            FInputChord ());

        UI_COMMAND (MenuCommand2,
            "Export VisLayer Actors Name", "Export All Actors Name by Visualable Layers.",
            EUserInterfaceActionType::Button,
            FInputChord ());

        UI_COMMAND (MenuCommand3,
            "Export Selected Actors Name", "Export All Actors Name by Your selections.",
            EUserInterfaceActionType::Button,
            FInputChord ());

    };

public:
    /* Command Action Objects */
    TSharedPtr<FUICommandInfo> MenuCommand1;
    TSharedPtr<FUICommandInfo> MenuCommand2;
    TSharedPtr<FUICommandInfo> MenuCommand3;
};

class FTestClassTemp_Base :public TSharedFromThis<FTestClassTemp_Base>
{
public:
    virtual  ~FTestClassTemp_Base () = default;
    virtual void Initialize ()=0;
    virtual void Unload () = 0;
};

class FTestClassTemp_01 : public FTestClassTemp_Base, public IHasMenuExtensibility
{
public:
    FTestClassTemp_01 () {};

    FName ClassName;
    TSharedPtr<FTestClassTemp_01> Instance;
    TSharedPtr<FExtensibilityManager> AllMenuExtensibilityManager;

    virtual void Initialize () override
    {
        Instance = MakeShareable(new FTestClassTemp_01 ());
        ClassName = FName (TEXT ("FTestClassTemp_01"));
        AllMenuExtensibilityManager = MakeShareable (new FExtensibilityManager);
        PrintMe ();
        LoadLayerExtender ();
    };

    virtual void Unload () override
    {
        if (!Instance.IsValid ())
        {
            Instance.Reset ();
        }
    };

    virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager () override
    {
        return AllMenuExtensibilityManager;
    };

    TSharedRef<FTestClassTemp_01> GetRef () const
    {
        TSharedPtr<FTestClassTemp_Base> const TempPtr = Instance->AsShared ();
        TSharedPtr<FTestClassTemp_01> const ThisClassPtr = StaticCastSharedPtr<FTestClassTemp_01> (TempPtr);

        return ThisClassPtr.ToSharedRef ();
    };

    void PrintMe () const
    {
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s Is Working ~~"), *ClassName.ToString ());

    };

    void LoadLayerExtender () const
    {
        UE_LOG (LogMineCustomToolEditor, Error, TEXT ("Load LayerExtender !!!"));

        //  Registered Commands
        LayerMenuToolCommands::Register ();

        // Find Layer Module
        FLayersModule& LayersModule = 
            FModuleManager::LoadModuleChecked<FLayersModule> (TEXT ("Layers"));

        // Add Extension
       auto const ExtenstionDelegate =
           FLayersModule::FLayersMenuExtender::CreateStatic (&FTestClassTemp_01::ExtenderMenu);
       LayersModule.GetAllLayersMenuExtenders ().Add(ExtenstionDelegate);

    }

    static TSharedRef<FExtender> ExtenderMenu (const TSharedRef<FUICommandList> InCommandList)
    {
        //UE_LOG (LogMineCustomToolEditor, Error, TEXT ("ExtenderMenu !!!"));

        TSharedRef<FUICommandList> const CommandList = MakeShareable (new FUICommandList);

        /* Get MenuCommands Class instance */
        LayerMenuToolCommands const ToolCommands = LayerMenuToolCommands::Get ();

        /* Do Mapping CommandInfo to CommandList ! */
        CommandList->MapAction (
            ToolCommands.MenuCommand1,
            FExecuteAction::CreateStatic (&FTestClassTemp_01::ExportLayerName),
            FCanExecuteAction ());
        CommandList->MapAction (
            ToolCommands.MenuCommand2,
            FExecuteAction::CreateStatic (&FTestClassTemp_01::ExportSelectedLayerActorsName),
            FCanExecuteAction ());
        CommandList->MapAction (
            ToolCommands.MenuCommand3,
            FExecuteAction::CreateStatic (&FTestClassTemp_01::ExportSelectedActorsNames),
            FCanExecuteAction ());


        TSharedPtr<FExtender> const MenuExtender = MakeShareable (new FExtender);
        MenuExtender->AddMenuExtension (
            TEXT ("LayersSelection"),
            EExtensionHook::After,
            CommandList,
            FMenuExtensionDelegate::CreateStatic (&FTestClassTemp_01::ExtendMenuSection)
        );

        return  MenuExtender.ToSharedRef();
    }

    static void ExtendMenuSection (FMenuBuilder &MenuBuilder)
    {
        // UE_LOG (LogMineCustomToolEditor, Error, TEXT ("ExtendMenuSection !!!"));
        /* Get MenuCommands Class instance */
        LayerMenuToolCommands const ToolCommands = LayerMenuToolCommands::Get ();


        MenuBuilder.BeginSection (TEXT("MineLayerTool"), LOCTEXT ("MineLayerToolMenu", "Mine Custom Layer Menu"));
        // Add Real Menu
        // MenuBuilder.AddMenuSeparator (FName (TEXT ("Section_1")));
        /* Add CommandInfo to Menu */
        MenuBuilder.AddMenuEntry (ToolCommands.MenuCommand1);
        MenuBuilder.AddMenuEntry (ToolCommands.MenuCommand2);
        MenuBuilder.AddMenuEntry (ToolCommands.MenuCommand3);

        MenuBuilder.EndSection ();
    }

    static void ExportSelectedLayerActorsName ()
    {
        //FLayersModule &LayersModule =
        //    FModuleManager::LoadModuleChecked<FLayersModule> (TEXT ("Layers"));
        TWeakObjectPtr<ULayersSubsystem> const LayerSubSys = (GEditor->GetEditorSubsystem<ULayersSubsystem> ());

        if (LayerSubSys.IsValid ())
        {
            TArray<FName> AllLayerNames;
            FString StringArrayToCopy = "";
            LayerSubSys->AddAllLayerNamesTo (AllLayerNames);
            ULayer *Layer;
            /* Get AActor in layer */
            for (FName const LayerName : AllLayerNames) {
                LayerSubSys->TryGetLayer (LayerName, Layer);
                if (Layer->bIsVisible) {
                    TArray<AActor *> ActorsInLayer = LayerSubSys->GetActorsFromLayer (LayerName);
                    for (auto const Actor : ActorsInLayer) {
                        StringArrayToCopy.Append (Actor->GetName () + "\n");
                    }
                }
            }
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT (" Actors Name Export to CopyBoard"));
            FPlatformMisc::ClipboardCopy (*StringArrayToCopy);
        }
    }

    static void ExportLayerName ()
    {

        TWeakObjectPtr<ULayersSubsystem> const LayerSubSys = (GEditor->GetEditorSubsystem<ULayersSubsystem> ());

        if (LayerSubSys.IsValid())
        {
            TArray< FName > AllLayerNames;
            FString StringArrayToCopy = "";
            LayerSubSys->AddAllLayerNamesTo (AllLayerNames);
            for (int32 NameId = 0; NameId < AllLayerNames.Num (); NameId++) {
                UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Names: %s"), *AllLayerNames[NameId].ToString ());
                StringArrayToCopy.Append (AllLayerNames[NameId].ToString () + "\n");
            }
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT (" Layers Name Export to CopyBoard"));
            FPlatformMisc::ClipboardCopy (*StringArrayToCopy);
        }
    };

    static void ExportSelectedActorsNames ()
    {
        TArray< AActor * > CurrentlySelectedActors;
        FString StringArrayToCopy = "";
        for (FSelectionIterator It (GEditor->GetSelectedActorIterator ()); It; ++It) {
            auto &&Actor = static_cast<AActor *>(*It);
            StringArrayToCopy.Append (Actor->GetName () + "\n");

        }
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT (" Actors Name Export to CopyBoard"));
        FPlatformMisc::ClipboardCopy (*StringArrayToCopy);
    }
};

class FTestClassTemp_02 : public FTestClassTemp_Base
{
    FTestClassTemp_02 () {};
    TSharedPtr<FTestClassTemp_02> Instance;

    virtual void Initialize () override
    {
        Instance = MakeShareable (new FTestClassTemp_02 ());
        LoadLayerExtender ();
    };

    virtual void Unload () override
    {
        if (!Instance.IsValid ()) {
            Instance.Reset ();
        }
    };

    void LoadLayerExtender () const
    {
        //Get the Level Editor module
        FLevelEditorModule &LevelEditorModule = 
            FModuleManager::LoadModuleChecked<FLevelEditorModule> ("LevelEditor");
        //Get the Viewport menu context extension item
        //(the right-click menu extension item of the selected object in the Viewport),
        //which is a delegate array 
        auto &LVCMExtenders = 
            LevelEditorModule.GetAllLevelViewportContextMenuExtenders ();
        //Add a new extension item to the array.
        //When you right-click to select an Actor and pop up the menu,
        //it will traverse the array and execute the bound delegate method
        LVCMExtenders.Add (FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateRaw (this, &FTestClassTemp_02::LVCMExtender));

    };

    //Delegate method,
    //the first parameter is the command list,
    //the second is the selected Actor array,
    //return the menu expansion item (that is, display content)
    TSharedRef<FExtender> LVCMExtender (const TSharedRef<FUICommandList> CommandList, const TArray<AActor *> Actors)
    {

        //Create an extension item,
        //similar to the usage in the previous chapter,
        //use it to expand the menu item
        TSharedPtr<FExtender> Extender = MakeShareable (new FExtender ());
        //Set the display content of the menu extension item,
        //the bound delegate is the last chapter Created delegate method
        Extender->AddMenuExtension ("ActorAsset",
            EExtensionHook::After,
            CommandList,
            FMenuExtensionDelegate::CreateRaw (this, &FTestClassTemp_02::AddMenuExtension)
        );

        //Load LevelEditor module
        //FLevelEditorModule &LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule> ("LevelEditor");
        //auto& LVCMExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
        //LVCMExtenders.Pop();


        //If the last (we just created) Extender is removed,
        //it will be displayed after the first menu and will not be displayed next time Show this item 
        //Return to the extension item, it will be displayed in the menu bar 
        return Extender.ToSharedRef ();
    }

    void AddMenuExtension (FMenuBuilder &MenuBuilder)
    {
        
    }

};


class FTestCodeListener : public IMineCustomToolModuleListenerInterface
{
public:
    TArray<TSharedPtr<FTestClassTemp_Base>> InstancesArray;

    virtual void OnStartupModule () override
    {
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Load Temporality Codes !!!"));
        TSharedPtr<FTestClassTemp_Base> const LayerExportMenuInstance =
            MakeShareable (new FTestClassTemp_01);
        InstancesArray.Add (LayerExportMenuInstance);

        for (auto const Instance : InstancesArray)
        {
            Instance->Initialize ();
        }
    };

    virtual void OnShutdownModule () override
    {
        for (auto Instance : InstancesArray) {
            if (Instance.IsValid())
                Instance->Unload ();
        }
    };
};

#undef LOCTEXT_NAMESPACE