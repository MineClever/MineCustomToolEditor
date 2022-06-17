#pragma once
#include "MineMouduleDefine.h"
#include "AssetTypeActions_Base.h"
#include "CustomDataType/ExampleData.h"
#ifdef MINE_EDITOR_CUSTOM_DATA_CAN_REIMPORT
#include "EditorReimportHandler.h"
#endif

/*
 * Finally we need to register type actions in editor module.
 * We add an array CreatedAssetTypeActions to save all type actions we registered,
 * so we can unregister them properly when module is unloaded:
 */
class FExampleDataTypeActions : public FAssetTypeActions_Base
{
public:
    FExampleDataTypeActions (EAssetTypeCategories::Type InAssetCategory);

    // IAssetTypeActions interface
    virtual FText GetName () const override;
    virtual FColor GetTypeColor () const override;
    virtual UClass *GetSupportedClass () const override;
    virtual uint32 GetCategories () override;

    #ifdef MINE_EDITOR_CUSTOM_DATA_CAN_REIMPORT

        virtual bool HasActions (const TArray<UObject *> &InObjects) const override { return true; };
        virtual void GetActions (const TArray<UObject *> &InObjects, FMenuBuilder &MenuBuilder) override;

        // Extended Helper Functions
        void ExecuteReimport (TArray<TWeakObjectPtr<UExampleData>> Objects);

    #endif


    // End of IAssetTypeActions interface


private:
    EAssetTypeCategories::Type MyAssetCategory;
};