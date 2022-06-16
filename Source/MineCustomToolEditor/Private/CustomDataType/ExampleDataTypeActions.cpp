#include "CustomDataType/ExampleDataTypeActions.h"


FExampleDataTypeActions::FExampleDataTypeActions (EAssetTypeCategories::Type InAssetCategory)
    : MyAssetCategory (InAssetCategory)
{
}

FText FExampleDataTypeActions::GetName () const
{
    return FText::FromString ("Example Data");
}

FColor FExampleDataTypeActions::GetTypeColor () const
{
    return FColor (230, 205, 165);
}

UClass *FExampleDataTypeActions::GetSupportedClass () const
{
    return UExampleData::StaticClass ();
}

uint32 FExampleDataTypeActions::GetCategories ()
{
    return MyAssetCategory;
}