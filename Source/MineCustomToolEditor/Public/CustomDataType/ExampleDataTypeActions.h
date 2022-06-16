#pragma once
#include "AssetTypeActions_Base.h"
#include "CustomDataType/ExampleData.h"

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
    // End of IAssetTypeActions interface

private:
    EAssetTypeCategories::Type MyAssetCategory;
};