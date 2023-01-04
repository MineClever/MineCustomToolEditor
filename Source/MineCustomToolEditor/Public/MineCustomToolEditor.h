#pragma once
#include "MineMouduleDefine.h"
#include "UnrealEd.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"

// Ref to : https://lxjk.github.io/2019/10/01/How-to-Make-Tools-in-U-E.html

#ifdef MINE_EDITOR_LOAD_EXAMPLE_ACTOR
    #include "DetailsCustomization/ExampleActor.h"
    #include "DetailsCustomization/ExampleActorDetails.h"
#endif


#ifdef MINE_EDITOR_LOAD_EXAMPLE_CUSTOM_DATA
    #include "CustomDataType/ExampleDataTypeActions.h"
#endif

class FMineToolEditor : public IMineCustomToolModuleInterface
{
public:
    /** IModuleInterface implementation */


    virtual void StartupModule () override;
    virtual void ShutdownModule () override;

    virtual void AddModuleListeners () override;

    /* Get The module made by yourself */
    static inline FMineToolEditor & Get ()
    {
        return FModuleManager::LoadModuleChecked< FMineToolEditor > (CURRENT_CUSTOM_MODULE_NAME);
    }

    static inline bool IsAvailable ()
    {
        return FModuleManager::Get ().IsModuleLoaded (CURRENT_CUSTOM_MODULE_NAME);
    }

public:
    void AddMenuExtension (
        const FMenuExtensionDelegate &extensionDelegate,
        FName extensionHook,
        const TSharedPtr<FUICommandList> &CommandList = nullptr,
        EExtensionHook::Position position = EExtensionHook::Before);

    TSharedRef<FWorkspaceItem> GetMenuRoot () { return MenuRoot; };

protected:
    TSharedPtr<FExtensibilityManager> LevelEditorMenuExtensibilityManager;
    TSharedPtr<FExtender> MenuExtender;

    static TSharedRef<FWorkspaceItem> MenuRoot;

    void MakePulldownMenu (FMenuBarBuilder &menuBuilder);
    void FillPulldownMenu (FMenuBuilder &menuBuilder);

#ifdef MINE_EDITOR_LOAD_EXAMPLE_CUSTOM_DATA
    TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;
#endif


};
