#pragma once
#include "MineMouduleDefine.h"
#include "CoreMinimal.h"
#include "ISequencerModule.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"


namespace FMineSequencerBaseMenuAction_Internal
{
    class FMineSequencerBaseExtensionLoader : public IMineCustomToolModuleListenerInterface
    {
    private:
        static TSharedPtr<FExtensibilityManager> GetSequencerCtxMenuExt ()
        {
            // NOTE: Get SequencerModule && FExtensibilityManager
            const ISequencerModule &SequencerModule = FModuleManager::Get ().LoadModuleChecked<ISequencerModule> ("Sequencer");
            TSharedPtr<FExtensibilityManager> Manager = SequencerModule.GetObjectBindingContextMenuExtensibilityManager ();
            return Manager;
        }
        static TSharedPtr<FExtensibilityManager> GetSequencerBarExt ()
        {
            // NOTE: Get SequencerModule && FExtensibilityManager
            const ISequencerModule &SequencerModule = FModuleManager::Get ().LoadModuleChecked<ISequencerModule> ("Sequencer");
            TSharedPtr<FExtensibilityManager> Manager = SequencerModule.GetToolBarExtensibilityManager ();
            return Manager;
        }

    protected:
        TArray<TSharedPtr<FExtender>> CtxExtenderPtrArray;
        TArray<TSharedPtr<FExtender>> BarExtenderPtrArray;
        TSharedPtr<FExtensibilityManager> ObjBindCtxExtensibilityManager = FMineSequencerBaseExtensionLoader::GetSequencerCtxMenuExt ();
        TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager = FMineSequencerBaseExtensionLoader::GetSequencerBarExt ();

    public:

        virtual void OnStartupModule () override;
        virtual void OnShutdownModule () override;


    };

}

