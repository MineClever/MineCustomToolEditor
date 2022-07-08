#include <AssetMenuTools/FTextureAssetActionListener.h>

#include "AssetToolsModule.h"
#include "AssetMenuTools/FAssetsProcessorFormSelection.hpp"
#include "AssetCreateHelper/FMineTexture2DCreateHelper.hpp"

#define LOCTEXT_NAMESPACE "FTextureAssetActionListener"

/* Implementation of FUTextureAssetProcessor_AutoSetTexFormat */
namespace FUTextureAssetProcessor_AutoSetTexFormat_Internal
{
    class FUTextureAssetProcessor_AutoSetTexFormat :public TAssetsProcessorFormSelection_Builder<UTexture>
    {
    protected:
        static bool bTagRuleExist;
        static TSet<FString> TagRule_SRGB;
        static TSet<FString> TagRule_Normal;
        static TSet<FString> TagRule_Mask;
        static TSet<FString> TagRule_ForceLinear;

    public:
        FUTextureAssetProcessor_AutoSetTexFormat ()
        {
            if (!bTagRuleExist) {
                UpdateTagRules ();
            }
        }

        virtual void ProcessAssets (TArray<UTexture *> &Assets) override
        {
            for (auto TexIt = Assets.CreateConstIterator (); TexIt; ++TexIt) {
                UTexture *const Texture = *TexIt;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target Texture is : %s"), *Texture->GetPathName ());
                CallConvertProcessor (Texture);
            }

        }

        static void UpdateTagRules ()
        {
            TagRule_SRGB.Append (CreateRuleFStringArray (TEXT ("srgb")));
            TagRule_Normal.Append (CreateRuleFStringArray (TEXT ("normal")));
            TagRule_Mask.Append (CreateRuleFStringArray (TEXT ("mask")));
            TagRule_ForceLinear.Append (CreateRuleFStringArray (TEXT ("forcelinear")));
            bTagRuleExist = true;
        }

        static TArray<FString> CreateRuleFStringArray (const FName &RuleName)
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
                    ArrayCreateFunc (TEXT ("srgb,fx,col,color,d,diffuse,diff"));
                    break;
                } 
                if (RuleName == TEXT ("normal"))
                {
                    ArrayCreateFunc (TEXT ("normal,norm,nor,faxian,n"));
                    break;
                }
                if (RuleName == TEXT ("mask")) {
                    ArrayCreateFunc (TEXT ("op,o,mask,opacity,alpha,silk,grey"));
                    break;
                }
                if (RuleName == TEXT ("forcelinear")) {
                    ArrayCreateFunc (TEXT ("linear,arm"));
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

        static void ConvertProcessor (UTexture *PTexObj,const bool bConvertVirtualTex=false)
        {
#pragma region TextureObjectProperties
            bool bSRGB = false;
            bool bNorm = false;
            bool bMask = false;
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
            LTagsSet.Append (LTagsArray);

            /* Test SRGB map*/
            const TSet<FString> &&LFoundTag_SRGB = TagRule_SRGB.Intersect (LTagsSet);
            if (LFoundTag_SRGB.Num () > 0) bSRGB = true;


            /* Test Normal map */
            const TSet<FString> &&LFoundTag_Normal = TagRule_Normal.Intersect (LTagsSet);
            if (LFoundTag_Normal.Num () > 0) bNorm = true;

            /* Test Mask Map*/
            const TSet<FString> &&LFoundTag_Mask = TagRule_Mask.Intersect (LTagsSet);
            if (LFoundTag_Mask.Num () > 0) bMask = true;

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
                    break;
                }
                if (bSRGB)
                    PTexObj->SRGB = true;
                break;
            } //End Switch


            /* Size Checker */
            const UTexture2D * const Tex2D = Cast<UTexture2D> (PTexObj);
            int32 const MaxSize = Tex2D->PlatformData->SizeX >= Tex2D->PlatformData->SizeY ? Tex2D->PlatformData->SizeX : Tex2D->PlatformData->SizeY;

            if (MaxSize <= 512) bSmallSize = true;
            TEnumAsByte<TextureMipGenSettings> MipGenSettings = TextureMipGenSettings::TMGS_LeaveExistingMips;
            while (true)
            {
                TEnumAsByte<TextureMipGenSettings> const OriginSettings = PTexObj->MipGenSettings;
                if (bSmallSize)
                {
                    MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
                    break;
                }
                if (MaxSize >= 1024 && MaxSize< 2048)
                {
                    MipGenSettings = TextureMipGenSettings::TMGS_Sharpen0;
                    break;
                }
                if (MaxSize >= 2048 && MaxSize < 4096) {
                    MipGenSettings = TextureMipGenSettings::TMGS_FromTextureGroup;
                    break;
                } else
                {
                    MipGenSettings = OriginSettings;
                }
                break;
            }
            PTexObj->MipGenSettings = MipGenSettings;


            /* Format setting */
            TextureCompressionSettings LTempCompressionSettings = TextureCompressionSettings::TC_Default;
            while (true) {
                if (bSmallSize) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Displacementmap;
                    PTexObj->MipLoadOptions = ETextureMipLoadOptions::OnlyFirstMip;
                    break;
                }
                if (bForceLinear && !bNorm) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Displacementmap;
                }
                if (bForceLinear && bSRGB) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_BC7;
                }
                if (bForceLinear && bMask) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Grayscale;
                }
                if (bForceLinear) {
                    break;
                }
                if (bNorm && bForceLinear) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Displacementmap;
                    break;
                }
                if (bNorm) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Normalmap;
                    break;
                }
                // Default
                LTempCompressionSettings = TextureCompressionSettings::TC_Default;
                break;
            }
            PTexObj->CompressionSettings = LTempCompressionSettings;



            /* Process Virtual texture property */
            bool const bVirtualTex = PTexObj->IsCurrentlyVirtualTextured ();
            if (bVirtualTex && bConvertVirtualTex)
            {
                ConvertTextureVirtualTo2dTex (PTexObj);
            }

            /* Save Current Package */ 
            PTexObj->GetPackage ()->MarkPackageDirty ();
            UPackage::Save (PTexObj->GetPackage (),
                PTexObj,
                EObjectFlags::RF_Public | ::RF_Standalone,
                *PTexObj->GetPackage ()->GetName (),
                GError,
                nullptr,
                true,
                true,
                SAVE_NoError | SAVE_Async
            );
        }

        static void ConvertTextureVirtualTo2dTex (UTexture* const PTexObj)
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
                for (auto ImageFilePath : FilesToImport)
                {

                    // Make a new package name by Image path name
                    FString ImageName, PackageDirPath, UnusedPath, LongPackageName, CurrentPackageFullPath = PTexObj->GetOutermost ()->GetName ();
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

        } // End of ConvertTextureVirtualTo2dTex


    };

}

namespace FUTextureAssetProcessor_AutoSetTexFormat_Internal
{
    bool FUTextureAssetProcessor_AutoSetTexFormat::bTagRuleExist = false;
    TSet<FString> FUTextureAssetProcessor_AutoSetTexFormat::TagRule_SRGB = TSet<FString> ();
    TSet<FString> FUTextureAssetProcessor_AutoSetTexFormat::TagRule_Normal = TSet<FString> ();
    TSet<FString> FUTextureAssetProcessor_AutoSetTexFormat::TagRule_Mask = TSet<FString> ();
    TSet<FString> FUTextureAssetProcessor_AutoSetTexFormat::TagRule_ForceLinear = TSet<FString> ();
}



/* Menu Commands */

namespace  FTextureAssetActionListener_Internal
{
    using namespace FUTextureAssetProcessor_AutoSetTexFormat_Internal;
    class MineAssetCtxMenuCommands final : public TCommands<MineAssetCtxMenuCommands>
    {
    public:

        /* INIT */
        static FName MenuCtxName;
        MineAssetCtxMenuCommands ()
            : TCommands<MineAssetCtxMenuCommands> (
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

            UI_COMMAND (MenuCommand1,
                "Auto set Tex Format",
                "Auto set format for selected texture assets.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );

        }

    public:
        /* Command Action Objects */
        TSharedPtr<FUICommandInfo> MenuCommand1;

    };
    FName MineAssetCtxMenuCommands::MenuCtxName = TEXT ("MineTexAssetCtxMenu");

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

            if (CheckSelectedTypeTarget<UTexture>(SelectedAssets,true)) {
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
            static const MineAssetCtxMenuCommands &ToolCommands = MineAssetCtxMenuCommands::Get ();
            MenuBuilder.AddMenuEntry (ToolCommands.MenuCommand1);
        }


        /**
         * @brief :                     Check if target type in current selections
         * @tparam :                    Should derived from UObject
         * @param   SelectedAssets :    Current Selections
         * @param   bCanCast :          Test if Asset Can cast to target Type
         * @return :                    return true if target type in current selections
         */
        template<typename T>
        static bool CheckSelectedTypeTarget (const TArray<FAssetData> &SelectedAssets, bool bCanCast = false)
        {
            bool bCurrentType = false;
            bool bCanCastType = false;
            for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
                const FAssetData &Asset = *AssetIt;
                if (bCanCast) bCanCastType = Cast<T> (Asset.GetAsset ()) != nullptr;
                bCurrentType = bCurrentType || (Asset.AssetClass == T::StaticClass ()->GetFName ()) || bCanCastType;
            }
            return bCurrentType;
        }

        //static FAssetsProcessorFormSelection_Base * &
        template<typename P>
        static TSharedPtr<FAssetsProcessorFormSelection_Base>
            CreateProcessorPtr (const TArray<FAssetData> &SelectedAssets)
        {
            static_assert (
                std::is_base_of_v<FAssetsProcessorFormSelection_Base, P>,
                "Must be derived from FAssetsProcessorFormSelection_Base"
                );

            TSharedPtr<P> Processor = MakeShareable (new P);//On Heap
            Processor->SelectedAssets = SelectedAssets;
            return StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (Processor);
        }


        static void MappingCommand (
            const TSharedPtr<FUICommandList> &CommandList,
            const TArray<FAssetData> &SelectedAssets
        )
        {
            static const MineAssetCtxMenuCommands &ToolCommands = MineAssetCtxMenuCommands::Get ();
            CommandList->MapAction (
                ToolCommands.MenuCommand1,
                FExecuteAction::CreateStatic (
                    &ExecuteProcessor,
                    CreateProcessorPtr<FUTextureAssetProcessor_AutoSetTexFormat> (SelectedAssets)
                ),
                FCanExecuteAction ()
            );
        }
    };

};



/* Load to module */

void FTextureAssetActionListener::InstallHooks ()
{
    UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install Common Asset Menu Hook"));
    // register commands
    FTextureAssetActionListener_Internal::MineAssetCtxMenuCommands::Register ();

    // Declare Delegate 
    ContentBrowserExtenderDelegate =
        FContentBrowserMenuExtender_SelectedAssets::CreateStatic (
            &FTextureAssetActionListener_Internal::
            FMineContentBrowserExtensions_SelectedAssets::
            OnExtendContentBrowserAssetSelectionMenu
        );

    // Get all content module delegates
    TArray<FContentBrowserMenuExtender_SelectedAssets>
        &CBMenuExtenderDelegates = GetExtenderDelegates ();

    // Add The delegate of mine
    CBMenuExtenderDelegates.Add (ContentBrowserExtenderDelegate);
    ContentBrowserExtenderDelegateHandle =
        CBMenuExtenderDelegates.Last ().GetHandle ();

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

TArray<FContentBrowserMenuExtender_SelectedAssets> &
FTextureAssetActionListener::GetExtenderDelegates ()
{
    /////////////////////////////
    ///Get ContentBrowser Module
    /////////////////////////////
    FContentBrowserModule &ContentBrowserModule =
        FModuleManager::LoadModuleChecked<FContentBrowserModule> (TEXT ("ContentBrowser"));

    return ContentBrowserModule.GetAllAssetViewContextMenuExtenders ();
}

#undef LOCTEXT_NAMESPACE