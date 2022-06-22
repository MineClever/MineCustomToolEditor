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

    };

public:
    /* Command Action Objects */
    TSharedPtr<FUICommandInfo> MenuCommand1;
    TSharedPtr<FUICommandInfo> MenuCommand2;
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