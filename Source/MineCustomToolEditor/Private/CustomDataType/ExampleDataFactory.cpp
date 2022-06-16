#include "CustomDataType/ExampleDataFactory.h"

UExampleDataFactory::UExampleDataFactory (const FObjectInitializer &ObjectInitializer) : Super (ObjectInitializer)
{
    SupportedClass = UExampleData::StaticClass ();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject *UExampleDataFactory::FactoryCreateNew (
    UClass *Class, UObject *InParent,
    FName Name,
    EObjectFlags Flags,
    UObject *Context,
    FFeedbackContext *Warn
) {
    UExampleData *NewObjectAsset = NewObject<UExampleData> (InParent, Class, Name, Flags | RF_Transactional);
    return NewObjectAsset;
}