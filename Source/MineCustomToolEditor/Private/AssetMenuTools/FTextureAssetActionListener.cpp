#include <AssetMenuTools/FTextureAssetActionListener.h>

#include "EditorStyleSet.h"
#include "AssetCreateHelper/FMinePackageSaveHelper.h"
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include "AssetCreateHelper/FMineTexture2DCreateHelper.hpp"
#include "AssetMenuTools/TAssetsProcessorFormSelection.hpp"
#include "AssetMenuTools/TMineContentBrowserExtensions_SelectedAssets_Base.h"
#include "ConfigIO/ConfigIO.h"

#define LOCTEXT_NAMESPACE "FTextureAssetActionListener"

/* Implementation of FUTextureAssetProcessor_AutoSetTexFormat */
namespace FUTextureAssetProcessor_AutoSetTexFormat_Internal
{
    using namespace MineFormatStringInternal;

    class FUTextureAssetProcessor_SetAs_Base :public TAssetsProcessorFormSelection_Builder<UTexture>
    {
    public:
        static auto MakeSrgbColorSpaceConvertSettings()
        {
            static auto SourceColSettings = FTextureSourceColorSettings();
            SourceColSettings.ChromaticAdaptationMethod = ETextureChromaticAdaptationMethod::TCAM_Bradford;
            SourceColSettings.EncodingOverride = ETextureSourceEncoding::TSE_sRGB;
            SourceColSettings.ColorSpace = ETextureColorSpace::TCS_sRGB;

            // Force set coordinate to convert !
            SourceColSettings.RedChromaticityCoordinate = FVector2D(0.64000, 0.33000);
            SourceColSettings.GreenChromaticityCoordinate = FVector2D(0.30000, 0.60000);
            SourceColSettings.BlueChromaticityCoordinate = FVector2D(0.15000, 0.06000);
            SourceColSettings.WhiteChromaticityCoordinate = FVector2D(0.31270, 0.32900);
            return SourceColSettings;
        }

        static auto MakeDefaultColorSpaceConvertSettings()
        {
            static auto SourceColSettings = FTextureSourceColorSettings();
            SourceColSettings.ChromaticAdaptationMethod = ETextureChromaticAdaptationMethod::TCAM_None;
            SourceColSettings.EncodingOverride = ETextureSourceEncoding::TSE_None;
            SourceColSettings.ColorSpace = ETextureColorSpace::TCS_None;
            return SourceColSettings;
        }


    protected:

        virtual void ProcessAssets (TArray<UTexture *> &Assets) override
        {
            TArray<UObject *> ObjectsToSave;
            TArray<UPackage * > PackagesToReload;
            for (auto TexIt = Assets.CreateConstIterator (); TexIt; ++TexIt) {
                UTexture *const Texture = *TexIt;
                if (!Texture->CanModify())
                {
                    //Skip if not modify valid
                    continue;
                }
                Texture->Modify ();
                Texture->DeferCompression = true;
                ProcessTexture (Texture);
                ObjectsToSave.Add (Texture);
                PackagesToReload.Add (Texture->GetPackage ());
            }
            UPackageTools::SavePackagesForObjects (ObjectsToSave);
            UPackageTools::ReloadPackages (PackagesToReload);
        }

        virtual void ProcessTexture (UTexture *const &Texture) = 0;


    };

    class FUTextureAssetProcessor_SetAsLinearMask final : public FUTextureAssetProcessor_SetAs_Base
    {
        virtual void ProcessTexture (UTexture* const& Texture) override
        {
            Texture->CompressionSettings = TextureCompressionSettings::TC_Masks;
            Texture->SRGB = false;
        }
    };

    class FUTextureAssetProcessor_SetAsNormal final : public FUTextureAssetProcessor_SetAs_Base
    {
        virtual void ProcessTexture (UTexture* const &Texture) override
        {
            Texture->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
            Texture->SRGB = false;
        }
    };

    class FUTextureAssetProcessor_FlipY final : public FUTextureAssetProcessor_SetAs_Base
    {
        virtual void ProcessTexture (UTexture* const &Texture) override
        {
            Texture->bFlipGreenChannel = !static_cast<bool>(Texture->bFlipGreenChannel);
        }
    };

    class FUTextureAssetProcessor_SetAsSRGB_On final : public FUTextureAssetProcessor_SetAs_Base
    {
        virtual void ProcessTexture (UTexture* const &Texture) override
        {
            Texture->SRGB = true;
            static auto const ConfigSettings = GetDefault<UMineEditorConfigSettings>();
            bool const bSetColorSpace = ConfigSettings->bSetSrgbColorSpace;

            if (bSetColorSpace)
            {
                Texture->SourceColorSettings = MakeSrgbColorSpaceConvertSettings();
            }

        }
    };

    class FUTextureAssetProcessor_SetAsSRGB_Off final : public FUTextureAssetProcessor_SetAs_Base
    {
        virtual void ProcessTexture (UTexture *const &Texture) override
        {
            Texture->SRGB = false;
            auto const ConfigSettings = GetDefault<UMineEditorConfigSettings>();
            
            bool const bSetColorSpace = ConfigSettings->bSetSrgbColorSpace;

            if (bSetColorSpace)
            {
                Texture->SourceColorSettings = MakeDefaultColorSpaceConvertSettings();
            }

        }
    };

    class FUTextureAssetProcessor_AutoSetTexFormat :public TAssetsProcessorFormSelection_Builder<UTexture>
    {
    protected:
        bool bTagRuleExist;
        TSet<FString> TagRule_SRGB;
        TSet<FString> TagRule_Normal;
        TSet<FString> TagRule_Mask;
        TSet<FString> TagRule_Grey;
        TSet<FString> TagRule_ForceLinear;

    public:
        FUTextureAssetProcessor_AutoSetTexFormat ():bTagRuleExist(false)
        {
            this->FUTextureAssetProcessor_AutoSetTexFormat::UpdateTagRules ();
        }

        virtual void ProcessAssets (TArray<UTexture *> &Assets) override
        {
            for (auto TexIt = Assets.CreateConstIterator (); TexIt; ++TexIt) {
                UTexture *const Texture = *TexIt;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target Texture is : %s"), *Texture->GetPathName ());
                CallConvertProcessor (Texture);
            }
        }

        virtual void UpdateTagRules ()
        {
            TagRule_SRGB.Empty ();
            TagRule_Normal.Empty ();
            TagRule_Mask.Empty ();
            TagRule_Grey.Empty ();
            TagRule_ForceLinear.Empty ();

            TagRule_SRGB.Append (CreateRuleFStringArray (TEXT ("srgb")));
            TagRule_Normal.Append (CreateRuleFStringArray (TEXT ("normal")));
            TagRule_Mask.Append (CreateRuleFStringArray (TEXT ("mask")));
            TagRule_Grey.Append (CreateRuleFStringArray (TEXT ("grey")));
            TagRule_ForceLinear.Append (CreateRuleFStringArray (TEXT ("forcelinear")));
            bTagRuleExist = true;
        }

        /**
         * @brief : Build TArray<FString> by inputted rule name. like "mask"
         * @param RuleName :FName
         * @return :TArray<FString>
         */
        virtual TArray<FString> CreateRuleFStringArray (const FName &RuleName)
        {
            /*
             * TODO:
             * Read rules form ini...
             * Store changed rules to ini file
             * Reset rules
             */
            static TArray<FString> LTempStringArray;
            LTempStringArray.Empty ();

            /* Lambda Function to create FString Array */
            static auto ArrayCreateFunc = [&](const FString InputText)
            {
                InputText.ParseIntoArray (LTempStringArray, TEXT (","), true);
            };

            while (true) {
                auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
                bool const bCustom = ConfigSettings->bUseCustomTexFormatConfig;

                if (RuleName == TEXT ("srgb")) 
                {
                    ArrayCreateFunc (bCustom? ConfigSettings->ConfigTexSrgbTags : TEXT ("srgb,fx,col,color,d,diffuse,diff,basecolor"));
                    break;
                } 
                if (RuleName == TEXT ("normal"))
                {
                    ArrayCreateFunc (bCustom ? ConfigSettings->ConfigTexNormalTags : TEXT ("normal,norm,nor,faxian,n"));
                    break;
                }
                if (RuleName == TEXT ("grey")) {
                    ArrayCreateFunc (bCustom ? ConfigSettings->ConfigTexSingleChannelTags : TEXT ("op,o,grey,opacity,alpha"));
                    break;
                }
                if (RuleName == TEXT ("mask")) {
                    ArrayCreateFunc (bCustom ? ConfigSettings->ConfigTexMaskChannelsTags : TEXT ("silk,arm"));
                    break;
                }
                if (RuleName == TEXT ("forcelinear")) {
                    ArrayCreateFunc (bCustom ? ConfigSettings->ConfigTexMaskChannelsTags : TEXT ("linear,arm,amibient,metalness,roughness,opacity,op"));
                    break;
                }
                if (RuleName == TEXT ("floating")) {
                    ArrayCreateFunc (bCustom ? ConfigSettings->ConfigTexHdrTags : TEXT ("hdr,hdri,floating"));
                    break;
                }
                break;
            };
            return LTempStringArray;
        }

        virtual void CallConvertProcessor(UTexture *PTexObj)
        {
            ConvertProcessor (PTexObj,true);
        }

    protected:
        virtual void ConvertProcessor (UTexture *PTexObj, const bool bConvertVirtualTex=false, bool const bNormAsMask=false) const
        {
            #pragma region TextureObjectProperties
            bool bSRGB = false;
            bool bNorm = false;
            bool bMask = false;
            bool bGrey = false;
            bool bForceLinear = false;
            bool bSmallSize = false;
            #pragma endregion TextureObjectProperties

            PTexObj->Modify ();
            // DeferCompression is more easy to change properties
            PTexObj->DeferCompression = true;

            FString &&LTexName = PTexObj->GetName ();
            TArray<FString> LTagsArray;
            TSet<FString> LTagsSet;

            // "T_Tex.Name.1001"-->"T_Tex_Name_1001"
            LTexName = LTexName.Replace (TEXT ("."), TEXT ("_"), ESearchCase::IgnoreCase);
            LTexName.ParseIntoArray (LTagsArray, TEXT ("_"), true);
            UE_LOG(LogMineCustomToolEditor,Log,TEXT("Found Name tags : %s"),*LTexName)
            LTagsSet.Append (LTagsArray);

            // Have to make rules to match!
            assert (bTagRuleExist);

            /* Test SRGB map*/
            const TSet<FString> &&LFoundTag_SRGB = TagRule_SRGB.Intersect (LTagsSet);
            if (LFoundTag_SRGB.Num () > 0) bSRGB = true;

            /* Test Normal map */
            const TSet<FString> &&LFoundTag_Normal = TagRule_Normal.Intersect (LTagsSet);
            if (LFoundTag_Normal.Num () > 0) bNorm = true;

            /* Test Mask Map*/
            const TSet<FString> &&LFoundTag_Mask = TagRule_Mask.Intersect (LTagsSet);
            if (LFoundTag_Mask.Num () > 0) bMask = true;

            /* Test Grey map*/
            const TSet<FString> &&LFoundTag_Grey = TagRule_Grey.Intersect (LTagsSet);
            if (LFoundTag_Grey.Num () > 0) bGrey = true;

            /* Test Force linear map */
            const TSet<FString> &&LFoundTag_ForceLinear = TagRule_ForceLinear.Intersect (LTagsSet);
            if (LFoundTag_ForceLinear.Num () > 0) bForceLinear = true;

            /* SRGB Setting */
            while (true) {
                if (
                    bForceLinear
                    || bNorm
                    || bMask
                    || (bSRGB == false)
                    || (bSRGB && bForceLinear)
                    ) {
                    PTexObj->SRGB = false;
                    PTexObj->SourceColorSettings = FUTextureAssetProcessor_SetAs_Base::MakeDefaultColorSpaceConvertSettings();
                    /* Legacy Gamma? */
                    PTexObj->bUseLegacyGamma = false;
                    break;
                }
                if (bSRGB)
                {
                    PTexObj->SRGB = true;
                    PTexObj->SourceColorSettings = FUTextureAssetProcessor_SetAs_Base::MakeSrgbColorSpaceConvertSettings();
                }

                break;
            } //End Switch


            /* Size Checker */
            const UTexture2D * const Tex2D = Cast<UTexture2D> (PTexObj);
            uint32 SizeX, SizeY;
            if (Tex2D!=nullptr)
            {
                const FTexturePlatformData* && LPlatformData = Tex2D->GetPlatformData();
                SizeX = LPlatformData->SizeX;
                SizeY = LPlatformData->SizeY;
            }
            else
            {
                const FTextureResource* &&TexRes = PTexObj->GetResource();
                SizeX = TexRes->GetSizeX ();
                SizeY = TexRes->GetSizeY ();
            }

            uint32 const MaxSize = SizeX >= SizeY ? SizeX : SizeY;
            if (MaxSize <= 512) bSmallSize = true;


            TEnumAsByte<TextureMipGenSettings> MipGenSettings;
            while (true)
            {
                TEnumAsByte<TextureMipGenSettings> const OriginSettings = PTexObj->MipGenSettings;
                if (bSmallSize)
                {
                    MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
                    break;
                }
                if (MaxSize > 512 && MaxSize< 1024)
                {
                    MipGenSettings = TextureMipGenSettings::TMGS_Sharpen0;
                    break;
                }
                if (MaxSize >= 1024 && MaxSize < 4096) {
                    MipGenSettings = TextureMipGenSettings::TMGS_FromTextureGroup;
                    break;
                }
                MipGenSettings = OriginSettings;
                break;
            }
            PTexObj->MipGenSettings = MipGenSettings;


            /* Format setting */
            TextureCompressionSettings LTempCompressionSettings = TextureCompressionSettings::TC_Default;
            ETextureLossyCompressionAmount LTempLossyCompression = PTexObj->LossyCompressionAmount;
            while (true) {
                if (bSmallSize) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
                    LTempLossyCompression = ETextureLossyCompressionAmount::TLCA_None;
                    PTexObj->MipLoadOptions = ETextureMipLoadOptions::OnlyFirstMip;
                    break;
                }
                if (bForceLinear && !bNorm) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
                }
                if (bForceLinear && bNorm) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_BC7;
                    LTempLossyCompression = ETextureLossyCompressionAmount::TLCA_None;
                }
                if (bForceLinear && bSRGB) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_BC7;
                }
                if (bForceLinear && bMask) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Masks;
                }
                if (bForceLinear && bGrey) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Alpha;
                }
                if (bForceLinear) {
                    break;
                }
                if (bNorm) {
                    LTempCompressionSettings = bNormAsMask ? TextureCompressionSettings::TC_BC7 : TextureCompressionSettings::TC_Normalmap;
                    if (bNormAsMask) {
                        LTempLossyCompression = ETextureLossyCompressionAmount::TLCA_None;
                    }
                    break;
                }
                if (bMask && !bSRGB)
                {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Masks;
                    LTempLossyCompression = ETextureLossyCompressionAmount::TLCA_None;
                    break;
                }
                if (bMask && bSRGB) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_BC7;
                    break;
                }
                if (bGrey && !bSRGB) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Alpha;
                    LTempLossyCompression = ETextureLossyCompressionAmount::TLCA_None;
                    break;
                }
                if (bGrey && bSRGB) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Grayscale;
                    break;
                }

                // Default
                LTempCompressionSettings = TextureCompressionSettings::TC_Default;
                break;
            }
            PTexObj->CompressionSettings = LTempCompressionSettings;
            PTexObj->LossyCompressionAmount = LTempLossyCompression;


            /* Process Virtual texture property */
            bool const bVirtualTex = PTexObj->IsCurrentlyVirtualTextured ();

            const FString &&TempString =
                FormattedFStringHelper::FormattedFString (
                    TEXT ("SizeX:{0}, SizeY:{1}, Small:{7}, SRGB:{2}, ForceLinear:{3}, Grey:{5}, Normal:{6}, VirtualTexture:{4} \n"),
                    SizeX, SizeY, bSRGB, bForceLinear, bVirtualTex, bGrey, bNorm, bSmallSize);
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"), *TempString)

            if (bVirtualTex && bConvertVirtualTex)
            {
                ConvertVirtualTexToTex2d (PTexObj, bNormAsMask);
                MinePackageHelperInternal::SaveUObjectPackage(PTexObj);
            }

            /* Save Current Package */
            MinePackageHelperInternal::SaveUObjectPackage (PTexObj);
        }

        virtual void ConvertVirtualTexToTex2d (UTexture* const PTexObj,  bool const bNormAsMask = false) const
        {
            using namespace MineAssetCreateHelperInternal;
            //const FAssetToolsModule &AssetToolsModule =
            //    FModuleManager::Get ().LoadModuleChecked<FAssetToolsModule> ("AssetTools");

            // Find all inputted path
            const UAssetImportData* TexImportData = Cast<UAssetImportData>(PTexObj->AssetImportData);
            const TArray<FString> FilesToImport = TexImportData->ExtractFilenames ();

            if (FilesToImport.Num()<=1)
            {
                // Reflector Method
                PTexObj->VirtualTextureStreaming = 0;
                //FObjectEditorUtils::SetPropertyValue (PTexObj,TEXT("VirtualTextureStreaming"),0);
            }
            else
            {
                FString const CurrentPackageFullPath = PTexObj->GetOutermost ()->GetName ();
                for (auto ImageFilePath : FilesToImport)
                {

                    // Make a new package name by Image path name
                    FString ImageName, PackageDirPath, UnusedPath, LongPackageName;
                    FPaths::Split (ImageFilePath, UnusedPath, ImageName, UnusedPath);
                    FPaths::Split (CurrentPackageFullPath, PackageDirPath, UnusedPath, UnusedPath);
                    ImageName = FPaths::MakeValidFileName(ImageName,'_').Replace(TEXT("."), TEXT("_"));
                    // FPackageName::TryConvertFilenameToLongPackageName (FPaths::ConvertRelativePathToFull (PackageDirPath, ImageName), LongPackageName);
                    LongPackageName = FPackageName::FilenameToLongPackageName(FPaths::ConvertRelativePathToFull(PackageDirPath, ImageName));

                    // Create New Texture Asset
                    UE_LOG (LogMineCustomToolEditor,Warning,TEXT("ImageFilePath : %s ,PackageName : %s"), *ImageFilePath, *LongPackageName);
                    UTexture2D* const NewTextureObject = FMineTextureAssetCreateHelper::CreateTexture(ImageFilePath, LongPackageName);

                    // Reformat
                    if (NewTextureObject != nullptr)
                        ConvertProcessor (NewTextureObject,false, bNormAsMask);
                }
            }
        } // End of ConvertVirtualTexToTex2d
    };

    class FUTextureAssetProcessor_AutoSetTexFormat_Pal :public FUTextureAssetProcessor_AutoSetTexFormat
    {
        virtual void CallConvertProcessor (UTexture *PTexObj) override
        {
            ConvertProcessor (PTexObj, true, true);
        }
    };
}



/* Menu Commands */
namespace  FTextureAssetActionListener_Internal
{
    using namespace FUTextureAssetProcessor_AutoSetTexFormat_Internal;
    using MineAssetCtxMenuCommands_DataMap = TMineContentBrowserExtensions_SelectedAssets_Internal::MineAssetCtxMenuCommands_CommandMap;

    class MineAssetCtxMenuCommandsInfo final : public TCommands<MineAssetCtxMenuCommandsInfo>, public MineAssetCtxMenuCommands_DataMap
    {
    public:

        /* INIT */
        static FName MenuCtxName;
        MineAssetCtxMenuCommandsInfo ()
            : TCommands<MineAssetCtxMenuCommandsInfo> (
                MenuCtxName, // Context name for fast lookup
                FText::FromString (MenuCtxName.ToString()), // Context name for displaying
                NAME_None,   // No parent context
                FEditorStyle::GetStyleSetName () // Icon Style Set
                )
        {
        }

        /* Register all commands in this class */
        virtual void RegisterCommands () override
        {

            // 0
            FORMAT_COMMAND_INFO (0, "Auto set Tex Format", "Auto set format for selected texture assets.", FUTextureAssetProcessor_AutoSetTexFormat);

            // 1
            FORMAT_COMMAND_INFO (1, "Auto set Pal Tex Format", "Auto set format for selected texture assets.Normal will set as mask.", FUTextureAssetProcessor_AutoSetTexFormat_Pal);

            // 2
            FORMAT_COMMAND_INFO (2, "Set sRGB ON", "Toggle sRGB ON.", FUTextureAssetProcessor_SetAsSRGB_On);

            // 3
            FORMAT_COMMAND_INFO (3, "Set sRGB OFF", "Toggle sRGB OFF.", FUTextureAssetProcessor_SetAsSRGB_Off);

            // 4
            FORMAT_COMMAND_INFO (4, "Set Masks Format", "set as Linear Masks format for selected texture assets.", FUTextureAssetProcessor_SetAsLinearMask);

            // 5
            FORMAT_COMMAND_INFO (5, "Set Normal Format", "set as NormalMap format for selected texture assets.", FUTextureAssetProcessor_SetAsNormal);

            // 6
            FORMAT_COMMAND_INFO (6, "Flip G", "Flip Green(Y) channal for selected texture assets.", FUTextureAssetProcessor_FlipY);


        }

    };

    FName MineAssetCtxMenuCommandsInfo::MenuCtxName = TEXT ("MineTexAssetCtxMenu");


    /* Extension to menu */
    template<typename TCommand>
    class FMineContentBrowserExtensions_SelectedAssets
    {
    public:
        static_assert(std::is_base_of_v<MineAssetCtxMenuCommands_DataMap, TCommand>, "Not a Valid TCommand Class. Must derived from MineAssetCtxMenuCommands_CommandMap.");

        static void ExecuteProcessor (
            TSharedPtr<FAssetsProcessorFormSelection_Base> const Processor
        )
        {
            Processor->Execute ();
        }

        static TSharedRef<FExtender>
        OnExtendContentBrowserAssetSelectionMenu (
            const TArray<FAssetData> &SelectedAssets
        )
        {
            static TSharedPtr<FUICommandList> CommandList;

            if (!CommandList.IsValid ())
                CommandList = MakeShareable (new FUICommandList);
            TSharedRef<FExtender> Extender (new FExtender ());

            MappingCommand (CommandList, SelectedAssets);

            if (AssetsProcessorCastHelper::CheckSelectedTypeTarget<UTexture>(SelectedAssets,true)) {
                // Add the Static actions sub-menu extender
                Extender->AddMenuExtension (
                    "GetAssetActions",
                    EExtensionHook::After,
                    CommandList,
                    FMenuExtensionDelegate::CreateStatic (
                        &CreateActionsSubMenu)
                );
            }
            return Extender;
        };

        static void CreateActionsSubMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            const FSlateIcon BaseMenuIcon = FSlateIcon ();
            const FText BaseMenuName = LOCTEXT ("ActionsSubMenuLabel", "Mine Texture Asset Actions");
            const FText BaseMenuTip = LOCTEXT ("ActionsSubMenuToolTip", "Type-related actions for Texture Asset.");

            MenuBuilder.AddSubMenu (
                BaseMenuName,
                BaseMenuTip,
                FNewMenuDelegate::CreateStatic (
                    &PopulateActionsMenu),
                false,
                BaseMenuIcon,
                true,
                FName (TEXT ("MineTexAssetsActions"))
            );

        }

        static void PopulateActionsMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            // Add to Menu
            static const TCommand &ToolCommandsInfo = TCommand::Get ();
            for (auto CommandInfo : ToolCommandsInfo.MenuCommandInfoActionMap)
            {
                MenuBuilder.AddMenuEntry (CommandInfo.Key);
            }
        }

        static void MappingCommand (
            const TSharedPtr<FUICommandList> &CommandList,
            const TArray<FAssetData> &SelectedAssets
        )
        {
            static const TCommand &ToolCommandsInfo = TCommand::Get ();
            for (auto CommandInfo : ToolCommandsInfo.MenuCommandInfoActionMap)
            {
                CommandList->MapAction (CommandInfo.Key,
                    FExecuteAction::CreateStatic (&ExecuteProcessor,CommandInfo.Value (SelectedAssets)),
                    FCanExecuteAction ()
                );
            }
        }
    };

};



/* Load to module */

void FTextureAssetActionListener::InstallHooks ()
{
    using namespace FTextureAssetActionListener_Internal;
    UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install Texture Asset Menu Hook"));
    // register commands
    MineAssetCtxMenuCommandsInfo::Register ();

    // Declare Delegate 
    ContentBrowserExtenderDelegate =
        FContentBrowserMenuExtender_SelectedAssets::CreateStatic (
            &FMineContentBrowserExtensions_SelectedAssets<MineAssetCtxMenuCommandsInfo>::OnExtendContentBrowserAssetSelectionMenu
        );

    // Get all content module delegates
    TArray<FContentBrowserMenuExtender_SelectedAssets>
        &CBMenuExtenderDelegates = GetExtenderDelegates ();

    // Add The delegate of mine
    CBMenuExtenderDelegates.Add (ContentBrowserExtenderDelegate);
    ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last ().GetHandle ();
}

void FTextureAssetActionListener::RemoveHooks ()
{
    TArray<FContentBrowserMenuExtender_SelectedAssets>
        &CBMenuExtenderDelegates = GetExtenderDelegates ();

    CBMenuExtenderDelegates.RemoveAll (
        [&](const FContentBrowserMenuExtender_SelectedAssets &Delegate)->bool {
            return Delegate.GetHandle () == ContentBrowserExtenderDelegateHandle;
        }
    );
}


#undef LOCTEXT_NAMESPACE