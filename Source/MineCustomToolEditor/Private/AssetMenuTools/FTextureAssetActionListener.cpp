#include "AssetMenuTools/FTextureAssetActionListener.h"
#include "AssetToolsModule.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "AssetMenuTools/FAssetsProcessorFormSelection.hpp"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/Private/VT/VirtualTextureBuiltData.h"
#include "Factories/TextureFactory.h"
#include "Misc/FileHelper.h"
#include "RHI.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
            bool bVirtual = false;
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

            /* Format setting */
            TextureCompressionSettings LTempCompressionSettings = TextureCompressionSettings::TC_Default;
            while (true) {
                if (bSmallSize) {
                    LTempCompressionSettings = TextureCompressionSettings::TC_Displacementmap;
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
        }

        static void ConvertTextureVirtualTo2dTex (UTexture* const PTexObj)
        {
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
                    UTexture2D* const NewTextureObject = CreateTexture(ImageFilePath, LongPackageName);

                    // Reformat
                    if (NewTextureObject != nullptr && NewTextureObject->IsValidLowLevel())
                        ConvertProcessor (NewTextureObject,false);
                }
            }
        } // End of ConvertTextureVirtualTo2dTex

        static UTexture2D *CreateTexture (const FString &LongPicturePath, const FString &LongPackageName)
        {

            // Get ImageName from LongPicturePath
            FString ImageName, ImageDirPath, ImageExtName;

            FPaths::Split (LongPicturePath, ImageDirPath, ImageName, ImageExtName);
            if (!FPlatformFileManager::Get ().GetPlatformFile ().FileExists (*LongPicturePath)) {
                UE_LOG(LogMineCustomToolEditor,Error,TEXT("Can't find image file : %s"), *LongPicturePath)
                return nullptr;
            }

            // Read Local Image to Texture
            FPixelFormatInfo const LPixelFormat = GPixelFormats[PF_B8G8R8A8];
            TArray<uint8> RawFileData;
            bool bStbLib = false;
            int SizeX =0, SizeY=0, ImgChannels=0;

            // Use Stb lib to load Tga file
            if (ImageExtName.Equals (TEXT ("tga"), ESearchCase::IgnoreCase))
                bStbLib = true;

            if (bStbLib)
            {
                TSharedPtr<unsigned char> const StbImgDataPtr = 
                    MakeShareable (stbi_load (TCHAR_TO_ANSI (*LongPicturePath), &SizeX , &SizeY, &ImgChannels, 4));
                if (StbImgDataPtr.IsValid ()) {
                    RawFileData.Append (StbImgDataPtr.Get (), SizeX  * SizeY * LPixelFormat.BlockBytes);
                    bStbLib = true;
                    stbi_image_free (StbImgDataPtr.Get ());
                } else bStbLib = false;
            }
            else if (!FFileHelper::LoadFileToArray (RawFileData, *LongPicturePath)) 
            {
                UE_LOG (LogMineCustomToolEditor, Error, TEXT ("Can't read image file : %s"), *LongPicturePath)
                return nullptr;
            }


            // Reformat Pixel Data
            if (RawFileData.Num()>0) {
                // Make Texture2D
                UTexture2D *Texture = nullptr;
                TArray<uint8> UncompressedRGBA;

                if (!bStbLib) // Use unreal ImageWrapper
                {
                    IImageWrapperModule &ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule> ("ImageWrapper");
                    EImageFormat const ImgInputFormat =
                        ImageWrapperModule.DetectImageFormat (RawFileData.GetData (), RawFileData.Num ());
                    TSharedPtr<IImageWrapper> const ImageWrapper = ImageWrapperModule.CreateImageWrapper (ImgInputFormat);

                    if (ImageWrapper.IsValid ())
                    {
                        bool const bHasSetCompressed = ImageWrapper->SetCompressed (RawFileData.GetData (), RawFileData.Num ());
                        bool const bHasGetRaw = ImageWrapper->GetRaw (ERGBFormat::BGRA, 8, UncompressedRGBA);
                        if (bHasSetCompressed && bHasGetRaw) // Use Unreal Method
                        {
                            SizeX = ImageWrapper->GetWidth ();
                            SizeY = ImageWrapper->GetHeight ();
                        }
                    } // End of ImageWrapper.IsValid ()
                } // End of (!bStbLib)
                else
                {
                    UncompressedRGBA = RawFileData;
                }



                // Make Sure Valid parameters
                if (SizeX <= 0 || SizeY <= 0 ||
                    (SizeX % LPixelFormat.BlockSizeX) != 0 ||
                    (SizeX % LPixelFormat.BlockSizeY) != 0) {
                    UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Invalid parameters specified for CreateTexture()"));
                    return nullptr;
                }

                // Create new texture pointer           
                UPackage *TexturePackage = CreatePackage (*LongPackageName);
                TexturePackage->FullyLoad ();

                Texture = CreateTexture (TexturePackage, UncompressedRGBA, SizeX, SizeY, LPixelFormat, FName (*ImageName));

                // Create Asset
                TexturePackage->MarkPackageDirty ();

                // Register Asset
                FAssetRegistryModule::AssetCreated (Texture);

                // Get Package File Name : path/TexAsset --> path/TexAsset.uasset
                FString &&PackageFileName = FPackageName::LongPackageNameToFilename (LongPackageName);

                //UPackage::SavePackage (TexturePackage, Texture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName.ToString());

                // Save!!
                UPackage::Save (TexturePackage,
                    Texture,
                    EObjectFlags::RF_Public | ::RF_Standalone,
                    *PackageFileName,
                    GError,
                    nullptr,
                    true,
                    true,
                    SAVE_NoError
                );
                return Texture;

            }
            else
            {
                UE_LOG (LogMineCustomToolEditor, Error, TEXT ("NoData has been loaded"));
                return nullptr;
            }

        }

        static UTexture2D * CreateTexture (UObject *Outer, const TArray<uint8> &PixelData, int32 &InSizeX, int32 &InSizeY, const FPixelFormatInfo & InPixelFormat, const FName &TextureName)
        {
            // Shamelessly copied from UTexture2D::CreateTransient with a few modifications

            UTexture2D *NewTexture = NewObject<UTexture2D> (Outer, TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
            NewTexture->AddToRoot ();
            FTexturePlatformData* const LPlatformData = new FTexturePlatformData ();
            NewTexture->PlatformData = LPlatformData;
            NewTexture->PlatformData->SizeX = InSizeX;
            NewTexture->PlatformData->SetNumSlices (1);
            NewTexture->PlatformData->SizeY = InSizeY;
            NewTexture->PlatformData->PixelFormat = InPixelFormat.UnrealFormat;

            //// Determine whether it is a power of 2 to use mipmap
            if ((InSizeX & (InSizeX - 1) || (InSizeY & (InSizeY - 1))))
                NewTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

            uint32 const PixelSize = InSizeX * InSizeY * InPixelFormat.BlockBytes;

            // Allocate first mipmap. (Mipmap0)
            FTexture2DMipMap* Mip = new FTexture2DMipMap ();
            NewTexture->PlatformData->Mips.Add (Mip);
            Mip->SizeX = InSizeX;
            Mip->SizeY = InSizeY;

            // Lock the texture so that it can be modified
            Mip->BulkData.Lock (LOCK_READ_WRITE);
            uint8* TextureDataPtr = static_cast<uint8*>(Mip->BulkData.Realloc(PixelSize));
            FMemory::Memcpy (TextureDataPtr, PixelData.GetData (), PixelSize); // copy to mip0
            Mip->BulkData.Unlock ();
            NewTexture->UpdateResource ();
            // To initialize the data in a non-transient field 
            NewTexture->Source.Init (InSizeX, InSizeY,
                1, 1,
                ETextureSourceFormat::TSF_BGRA8, PixelData.GetData ()
            );

            return NewTexture;
        }
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