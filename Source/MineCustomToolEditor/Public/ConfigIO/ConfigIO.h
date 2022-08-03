#pragma once
#include "MineMouduleDefine.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"
#include "ConfigIO.generated.h"

UCLASS(config = EditorUserSettings, defaultconfig)
class CURRENT_CUSTOM_MODULE_API UMineEditorConfigSettings : public UObject
{
    GENERATED_BODY ()

public:
    UPROPERTY(EditAnywhere, config, Category = "MineAlembicProxyMatch")
    bool bUseCustomProxyConfig = false;

    UPROPERTY (EditAnywhere, config, Category = "MineAlembicProxyMatch")
    FString ConfigAlembicSubDirMatchKey = TEXT ("Cloth");

    UPROPERTY (EditAnywhere, config, Category = "MineAlembicProxyMatch")
    FString ConfigAlembicPathRule = TEXT ("Animations/Alembic");

    UPROPERTY (EditAnywhere, config, Category = "MineAlembicProxyMatch")
    FString ConfigAlembicProxyMatPath = TEXT ("/Game/PalTrailer/MaterialLibrary/Base/Charactor/CFX_Material/Mat_Daili_Inst");
};


class FMineToolConfigLoader :public IMineCustomToolModuleListenerInterface
{
protected:
    struct FConfigSettingsName
    {
        FName ContainerName;
        FName CategoryName;
        FName SectionName;
        FText DisplayName;
        FText DescriptionName;
    };
public:
    FConfigSettingsName BaseSetting;
    virtual void OnStartupModule () override;
    virtual void OnShutdownModule () override;
};