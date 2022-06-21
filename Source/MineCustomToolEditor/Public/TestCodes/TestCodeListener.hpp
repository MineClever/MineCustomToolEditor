#pragma once
#include "Interfaces/IMineCustomToolModuleInterface.h"
#include "CoreMinimal.h"
#include "MineMouduleDefine.h"
#include "Layers/Public/LayersModule.h"

#define LOCTEXT_NAMESPACE "TestCodeListener"

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
        ClassName = FName (TEXT ("TestClass"));
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

    TSharedRef<FTestClassTemp_01> GetRef () const
    {
        TSharedPtr<FTestClassTemp_Base> const TempPtr = Instance->AsShared();
        TSharedPtr<FTestClassTemp_01> const ThisClassPtr = StaticCastSharedPtr<FTestClassTemp_01> (TempPtr);

        return ThisClassPtr.ToSharedRef();
    };

    virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager () override
    {
        /*ref: Source\Editor\LandscapeEditor\Private\LandscapeEditorModule.cpp line:98*/
        return AllMenuExtensibilityManager;
    };

    void PrintMe () const
    {
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s Is Working ~~"), *ClassName.ToString ());

    };

    void LoadLayerExtender () const
    {
        /*This method not work ! Fuck Unreal!*/
        UE_LOG (LogMineCustomToolEditor, Error, TEXT ("Load LayerExtender !!!"));
        FLayersModule& LayersModule = 
            FModuleManager::LoadModuleChecked<FLayersModule> (TEXT ("Layers"));

       auto const ExtenstionDelegate =
           FLayersModule::FLayersMenuExtender::CreateStatic (&FTestClassTemp_01::ExtenderMenu);
       LayersModule.GetAllLayersMenuExtenders ().Add(ExtenstionDelegate);
    }

    static void ExportLayerName ()
    {
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Test To Layers??"));
    };

    static TSharedRef<FExtender> ExtenderMenu (const TSharedRef<FUICommandList> InCommandList)
    {
        UE_LOG (LogMineCustomToolEditor, Error, TEXT ("ExtenderMenu !!!"));
        TSharedPtr<FExtender> const MenuExtender = MakeShareable (new FExtender);
        MenuExtender->AddMenuExtension (
            TEXT ("LayersSelection"),
            EExtensionHook::After,
            TSharedPtr<FUICommandList> (),
            FMenuExtensionDelegate::CreateStatic (&FTestClassTemp_01::ExtendMenuSection));
        return  MenuExtender.ToSharedRef();
    }

    static void ExtendMenuSection (FMenuBuilder &MenuBuilder)
    {
        UE_LOG (LogMineCustomToolEditor, Error, TEXT ("ExtendMenuSection !!!"));

        MenuBuilder.BeginSection (TEXT("MineLayerTool"), LOCTEXT ("MineLayerToolMenu", "Mine Custom Layer Menu"));
        // Add Real Menu
        // MenuBuilder.AddMenuSeparator (FName (TEXT ("Section_1")));
        MenuBuilder.AddMenuEntry (
            LOCTEXT ("ExporLayerName", "Export Layer Name"),
            LOCTEXT ("ExporLayerNameTooltip", "Export Layer Name."),
            FSlateIcon (),
            // NOTE 设置点击触发的函数
            FExecuteAction::CreateStatic (&FTestClassTemp_01::ExportLayerName),
            TEXT ("MineLayerTool"),
            EUserInterfaceActionType::Button
        );

        MenuBuilder.EndSection ();
    }

};

class FTestCodeListener : public IMineCustomToolModuleListenerInterface
{
public:
    TSharedPtr<FTestClassTemp_Base> TestInstance;

    virtual void OnStartupModule () override
    {
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Load Test Codes !!!"));
        TestInstance = MakeShareable (new FTestClassTemp_01);
        TestInstance->Initialize ();

    };

    virtual void OnShutdownModule () override
    {
        TWeakPtr<FTestClassTemp_Base> const WeakInstance = TestInstance;

        if (WeakInstance.IsValid())
        {
            TestInstance->Unload ();
        }
    };
};

#undef LOCTEXT_NAMESPACE