#pragma once
#include "MineMouduleDefine.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"
#include "ConfigIO.generated.h"

UCLASS(config = EditorUserSettings, defaultconfig)
class CURRENT_CUSTOM_MODULE_API UMineEditorConfigSettings : public UObject
{
    GENERATED_BODY ()

public:
    // All Alembic Proxy Configures
    ///////////////////////////////////////////////

    UPROPERTY(EditAnywhere, config, Category = "MineAlembicProxyMatch")
    bool bUseCustomProxyConfig = false;

    UPROPERTY (EditAnywhere, config, Category = "MineAlembicProxyMatch")
    FString ConfigAlembicSubDirMatchKey = TEXT ("Cloth");

    UPROPERTY (EditAnywhere, config, Category = "MineAlembicProxyMatch")
    FString ConfigAlembicPathRule = TEXT ("Animations/Alembic");

    UPROPERTY (EditAnywhere, config, Category = "MineAlembicProxyMatch")
    FString ConfigAlembicProxyMatPath = TEXT ("/Game/PalTrailer/MaterialLibrary/Base/Charactor/CFX_Material/Mat_Daili_Inst");

    // All Texture Setting Match rule
    ///////////////////////////////////////////////

    UPROPERTY (EditAnywhere, config, Category = "MineTextureFormat")
    bool bUseCustomTexFormatConfig = false;

    UPROPERTY (EditAnywhere, config, Category = "MineTextureFormat")
    FString ConfigTexSrgbTags = TEXT ("srgb,fx,col,color,d,diffuse,diff,basecolor");

    UPROPERTY (EditAnywhere, config, Category = "MineTextureFormat")
    FString ConfigTexNormalTags = TEXT ("normal,norm,nor,faxian,n");

    UPROPERTY (EditAnywhere, config, Category = "MineTextureFormat")
    FString ConfigTexSingleChannelTags = TEXT ("op,o,grey,opacity,alpha");

    UPROPERTY (EditAnywhere, config, Category = "MineTextureFormat")
    FString ConfigTexMaskChannelsTags = TEXT ("silk,arm");

    UPROPERTY (EditAnywhere, config, Category = "MineTextureFormat")
    FString ConfigTexForceLinearTags = TEXT ("linear,arm,amibient,metalness,roughness,opacity,op");

    UPROPERTY (EditAnywhere, config, Category = "MineTextureFormat")
    FString ConfigTexHdrTags = TEXT ("hdr,hdri,floating");

    // All StaticMesh Asset Configures
    ///////////////////////////////////////////////

    UPROPERTY (EditAnywhere, config, Category = "MineStaticMeshProcessor")
    bool bUseCustomStaticMeshProcessorConfig = false;

    UPROPERTY (EditAnywhere, config, Category = "MineStaticMeshProcessor")
    float ConfigDistanceFiledResolutionScale = 24;

    // All SkeletalMesh Asset Configures
    ///////////////////////////////////////////////


    // All Material Bind Configures
    ///////////////////////////////////////////////

    UPROPERTY (EditAnywhere, config, Category = "MineMaterialBindRule")
    bool bUseCustomMaterialBindConfig = false;

    UPROPERTY (EditAnywhere, config, Category = "MineMaterialBindRule")
    FString ConfigMaterialDirectoryRule = TEXT ("material,mat,materials");
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