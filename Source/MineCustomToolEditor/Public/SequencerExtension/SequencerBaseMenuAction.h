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
        TArray<FAssetEditorExtender> ExtenderDelegates;

    protected:
        TArray<FDelegateHandle> ExtenderHandles;

    public:

        virtual void OnStartupModule () override;
        virtual void OnShutdownModule () override;

        static TSharedPtr<FExtensibilityManager> GetSequencerExt ()
        {
            // NOTE: Get SequencerModule && FExtensibilityManager
            const ISequencerModule &SequencerModule = FModuleManager::Get ().LoadModuleChecked<ISequencerModule> ("Sequencer");
            TSharedPtr<FExtensibilityManager> Manager = SequencerModule.GetObjectBindingContextMenuExtensibilityManager ();
            return Manager;
        }
    };

}

