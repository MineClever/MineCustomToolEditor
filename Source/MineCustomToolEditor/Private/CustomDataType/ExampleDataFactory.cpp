#include "CustomDataType/ExampleDataFactory.h"


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

/*
 * Note we changed bCreateNew and bEditAfterNew to false.
 * We set "SourceFilePath" so we can do reimport later.
 * If you want to import binary file, set bText = false,
 * and override FactoryCreateBinary function instead.
 */
#ifdef  MINE_EDITOR_CUSTOM_DATA_CAN_REIMPORT
UExampleDataFactory::UExampleDataFactory (const FObjectInitializer &ObjectInitializer) : Super (ObjectInitializer)
{
    Formats.Add (TEXT ("txt;Custom Text Data"));
    SupportedClass = UExampleData::StaticClass ();
    bCreateNew = false; // turned off for import
    bEditAfterNew = false; // turned off for import
    bEditorImport = true;
    bText = true; // turned off for binary file
}
#else
UExampleDataFactory::UExampleDataFactory (const FObjectInitializer &ObjectInitializer) : Super (ObjectInitializer)
{
    SupportedClass = UExampleData::StaticClass ();
    bCreateNew = true;
    bEditAfterNew = true;
}
#endif

UObject *UExampleDataFactory::FactoryCreateText (
    UClass *InClass,
    UObject *InParent,
    FName InName,
    EObjectFlags Flags,
    UObject *Context,
    const TCHAR *Type,
    const TCHAR *&Buffer,
    const TCHAR *BufferEnd,
    FFeedbackContext *Warn
) {
    // FEditorDelegates::OnAssetPreImport.Broadcast (this, InClass, InParent, InName, Type);
    GEditor->GetEditorSubsystem<UImportSubsystem> ()->OnAssetPreImport.Broadcast (this, InClass, InParent, InName, Type);

    // if class type or extension doesn't match, return
    if (InClass != UExampleData::StaticClass () ||
        FCString::Stricmp (Type, TEXT ("txt")) != 0)
            return nullptr;

    UExampleData *Data = CastChecked<UExampleData> (NewObject<UExampleData> (InParent, InName, Flags));
    MakeExampleDataFromText (Data, Buffer, BufferEnd);

    // save the source file path
    Data->SourceFilePath = UAssetImportData::SanitizeImportFilename (CurrentFilename, Data->GetOutermost ());

    //FEditorDelegates::OnAssetPostImport.Broadcast (this, Data);
    GEditor->GetEditorSubsystem<UImportSubsystem> ()->OnAssetPostImport.Broadcast (this, Data);

    return Data;
}

bool UExampleDataFactory::FactoryCanImport (const FString &Filename)
{
    return FPaths::GetExtension (Filename).Equals (TEXT ("txt"));
}

void UExampleDataFactory::MakeExampleDataFromText (class UExampleData *Data, const TCHAR *&Buffer, const TCHAR *BufferEnd)
{
    Data->ExampleString = Buffer;
}