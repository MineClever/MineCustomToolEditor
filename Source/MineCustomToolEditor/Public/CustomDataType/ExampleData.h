#pragma once
#include "MineMouduleDefine.h"
#include "ExampleData.generated.h"

/*
 * For simple data,
 * you can just inherit from UDataAsset class,
 * then you can create your data object in Unreal content browser:
 * Add New ¡ú miscellaneous ¡ú Data Asset
 */
UCLASS(Blueprintable)
class CURRENT_CUSTOM_MODULE_API UExampleData : public UObject
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, Category = "Properties")
        FString ExampleString;

#if WITH_EDITORONLY_DATA
    UPROPERTY (Category = SourceAsset, VisibleAnywhere)
        FString SourceFilePath;
#endif
};