﻿#include "CustomDataType/ExampleDataTypeActions.h"


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

#ifdef MINE_EDITOR_CUSTOM_DATA_CAN_REIMPORT
void FExampleDataTypeActions::GetActions (const TArray<UObject *> &InObjects, FMenuBuilder &MenuBuilder)
{
    auto ExampleDataImports = GetTypedWeakObjectPtrs<UExampleData> (InObjects);

    MenuBuilder.AddMenuEntry (
        FText::FromString ("Reimport"),
        FText::FromString ("Reimports data."),
        FSlateIcon (),
        FUIAction (FExecuteAction::CreateSP (this,
            &FExampleDataTypeActions::ExecuteReimport,
            ExampleDataImports),
            FCanExecuteAction ()
        )
    );
}

void FExampleDataTypeActions::ExecuteReimport (TArray<TWeakObjectPtr<UExampleData>> Objects)
{
    for (auto ObjIt = Objects.CreateConstIterator (); ObjIt; ++ObjIt) {
        auto Object = (*ObjIt).Get ();
        if (Object) {
            FReimportManager::Instance ()->Reimport (Object, /*bAskForNewFileIfMissing=*/true);
        }
    }
}
#endif
