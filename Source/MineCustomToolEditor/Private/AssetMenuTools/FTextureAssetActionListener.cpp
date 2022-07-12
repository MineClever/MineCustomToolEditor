#include <functional>

#include <AssetMenuTools/FTextureAssetActionListener.h>

#include "AssetToolsModule.h"
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include "AssetMenuTools/FAssetsProcessorFormSelection.hpp"
#include "AssetCreateHelper/FMineTexture2DCreateHelper.hpp"
#include "AssetCreateHelper/FMinePackageSaveHelper.h"

#define LOCTEXT_NAMESPACE "FTextureAssetActionListener"

/* Implementation of FUTextureAssetProcessor_AutoSetTexFormat */
namespace FUTextureAssetProcessor_AutoSetTexFormat_Internal
{
    using namespace MineFormatStringInternal;

    class FUTextureAssetProcessor_SetAs_Base :public TAssetsProcessorFormSelection_Builder<UTexture>
    {
        virtual void ProcessAssets (TArray<UTexture *> &Assets) override
        {
            TArray<UObject *> ObjectsToSave;
            for (auto TexIt = Assets.CreateConstIterator (); TexIt; ++TexIt) {
                UTexture *const Texture = *TexIt;
                Texture->Modify ();
                ProcessTexture (Texture);
                ObjectsToSave.Add (Texture);
            }
            UPackageTools::SavePackagesForObjects (ObjectsToSave);
        }

        virtual void ProcessTexture(UTexture* const& Texture)
        {
            Texture->CompressionSettings = TextureCompressionSettings::TC_Masks;
        }
    };

    class FUTextureAssetProcessor_SetAsLinearMask final : public FUTextureAssetProcessor_SetAs_Base
    {
        virtual void ProcessTexture (UTexture* const& Texture) override
        {
            Texture->CompressionSettings = TextureCompressionSettings::TC_Masks;
        }
    };

    class FUTextureAssetProcessor_SetAsSRGB_On final : public FUTextureAssetProcessor_SetAs_Base
    {
        virtual void ProcessTexture (UTexture* const& Texture) override
        {
            Texture->SRGB = true;
        }
    };

    class FUTextureAssetProcessor_SetAsSRGB_Off final : public FUTextureAssetProcessor_SetAs_Base
    {
        virtual void ProcessTexture (UTexture *const &Texture) override
        {
            Texture->SRGB = false;
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

        virtual TArray<FString> CreateRuleFStringArray (const FName &RuleName)
        {
            /*
             * TODO:
             * Read rules form ini...
             */
            static TArray<FString> LTempStringArray;
            LTempStringArray.Empty ();

            /* Lambda Function to create FString Array */
            static auto ArrayCreateFunc = [&](const TCHAR *InputText)
            {
                FString &&TempString = InputText;
                TempString.ParseIntoArray (LTempStringArray, TEXT (","), true);
            };

            while (true) {
                if (RuleName == TEXT ("srgb")) 
                {
                    ArrayCreateFunc (TEXT ("srgb,fx,col,color,d,diffuse,diff,basecolor"));
                    break;
                } 
                if (RuleName == TEXT ("normal"))
                {
                    ArrayCreateFunc (TEXT ("normal,norm,nor,faxian,n"));
                    break;
                }
                if (RuleName == TEXT ("grey")) {
                    ArrayCreateFunc (TEXT ("op,o,grey,opacity,alpha"));
                    break;
                }
                if (RuleName == TEXT ("mask")) {
                    ArrayCreateFunc (TEXT ("silk,arm"));
                    break;
                }
                if (RuleName == TEXT ("forcelinear")) {
                    ArrayCreateFunc (TEXT ("linear,arm,amibient,metalness,roughness,opacity,op"));
                    break;
                }
                if (RuleName == TEXT ("floating")) {
                    ArrayCreateFunc (TEXT ("hdr,hdri,floating"));
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
        virtual void ConvertProcessor (UTexture *PTexObj, const bool bConvertVirtualTex=false, bool bNormAsMask=false) const
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
                    /* Legacy Gamma? */
                    PTexObj->bUseLegacyGamma = false;
                    break;
                }
                if (bSRGB)
                    PTexObj->SRGB = true;
                break;
            } //End Switch


            /* Size Checker */
            const UTexture2D * const Tex2D = Cast<UTexture2D> (PTexObj);
            uint32 SizeX, SizeY;
            if (Tex2D!=nullptr)
            {
                SizeX = Tex2D->PlatformData->SizeX;
                SizeY = Tex2D->PlatformData->SizeY;
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

                    LTempCompressionSettings = bNormAsMask ? TextureCompressionSettings ::TC_Masks : TextureCompressionSettings::TC_Normalmap;
                    break;
                }
                if (bMask && !bSRGB)
                {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Masks;
                    break;
                }
                if (bMask && bSRGB) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_BC7;
                    break;
                }
                if (bGrey && !bSRGB) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Alpha;
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
                    SizeX, SizeY, bSRGB, bForceLinear, bVirtualTex, bGrey, bSmallSize);
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"), *TempString)

            if (bVirtualTex && bConvertVirtualTex)
            {
                ConvertVirtualTexToTex2d (PTexObj);
            }

            /* Save Current Package */
            MinePackageHelperInternal::SaveUObjectPackage (PTexObj);
        }

        virtual void ConvertVirtualTexToTex2d (UTexture* const PTexObj) const
        {
            using namespace MineAssetCreateHelperInternal;
            //const FAssetToolsModule &AssetToolsModule =
            //    FModuleManager::Get ().LoadModuleChecked<FAssetToolsModule> ("AssetTools");

            // Find all inputted path
            const UAssetImportData * TexImportData = PTexObj->AssetImportData;
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
                    FPackageName::TryConvertFilenameToLongPackageName (FPaths::ConvertRelativePathToFull (PackageDirPath, ImageName), LongPackageName);

                    // Create New Texture Asset
                    UE_LOG (LogMineCustomToolEditor,Warning,TEXT("ImageFilePath : %s ,PackageName : %s"), *ImageFilePath, *LongPackageName);
                    UTexture2D* const NewTextureObject = FMineTextureAssetCreateHelper::CreateTexture(ImageFilePath, LongPackageName);

                    // Reformat
                    if (NewTextureObject != nullptr)
                        ConvertProcessor (NewTextureObject,false);
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
    using FBaseProcessorFunction = 
        TFunctionRef<TSharedPtr<FAssetsProcessorFormSelection_Base> (const TArray<FAssetData> &)>;

    class MineAssetCtxMenuCommandsInfo final : public TCommands<MineAssetCtxMenuCommandsInfo>
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
            using namespace AssetsProcessorCastHelper;
            // Build Array

            TSharedPtr<FUICommandInfo> MenuCommandInfo_0;
            UI_COMMAND (MenuCommandInfo_0,
                "Auto set Tex Format",
                "Auto set format for selected texture assets.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );
            MenuCommandInfoActionMap.Emplace (MenuCommandInfo_0, 
                CreateBaseProcessorPtr<FUTextureAssetProcessor_AutoSetTexFormat>);

            TSharedPtr<FUICommandInfo> MenuCommandInfo_1;
            UI_COMMAND (MenuCommandInfo_1,
                "Auto set Pal Tex Format",
                "Auto set format for selected texture assets.Normal will set as mask.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );
            MenuCommandInfoActionMap.Emplace (MenuCommandInfo_1,
                CreateBaseProcessorPtr<FUTextureAssetProcessor_AutoSetTexFormat_Pal>);

            TSharedPtr<FUICommandInfo> MenuCommandInfo_2;
            UI_COMMAND (MenuCommandInfo_2,
                "Set sRGB ON",
                "Toggle sRGB ON.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );
            MenuCommandInfoActionMap.Emplace (MenuCommandInfo_2,
                CreateBaseProcessorPtr<FUTextureAssetProcessor_SetAsSRGB_On>);

            TSharedPtr<FUICommandInfo> MenuCommandInfo_3;
            UI_COMMAND (MenuCommandInfo_3,
                "Set sRGB OFF",
                "Toggle sRGB OFF.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );
            MenuCommandInfoActionMap.Emplace (MenuCommandInfo_3,
                CreateBaseProcessorPtr<FUTextureAssetProcessor_SetAsSRGB_Off>);

            TSharedPtr<FUICommandInfo> MenuCommandInfo_4;
            UI_COMMAND (MenuCommandInfo_4,
                "Set Masks Format",
                "set as Linear Masks format for selected texture assets.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );
            MenuCommandInfoActionMap.Emplace (MenuCommandInfo_4,
                CreateBaseProcessorPtr<FUTextureAssetProcessor_SetAsLinearMask>);

        }

    public:
        /* Command Action Objects */
        TMap<TSharedPtr<FUICommandInfo>, FBaseProcessorFunction> MenuCommandInfoActionMap;
    };

    FName MineAssetCtxMenuCommandsInfo::MenuCtxName = TEXT ("MineTexAssetCtxMenu");

    /* Extension to menu */
    class FMineContentBrowserExtensions_SelectedAssets
    {
    public:

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
            static const MineAssetCtxMenuCommandsInfo &ToolCommandsInfo = MineAssetCtxMenuCommandsInfo::Get ();
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
            static const MineAssetCtxMenuCommandsInfo &ToolCommandsInfo = MineAssetCtxMenuCommandsInfo::Get ();
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
            &FMineContentBrowserExtensions_SelectedAssets::OnExtendContentBrowserAssetSelectionMenu
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