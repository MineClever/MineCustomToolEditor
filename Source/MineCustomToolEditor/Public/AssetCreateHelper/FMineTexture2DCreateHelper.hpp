#pragma once
#include "MineMouduleDefine.h"
#include <AssetRegistry/AssetRegistryModule.h>
#include "Engine/Private/VT/VirtualTextureBuiltData.h"
#include "Factories/TextureFactory.h"
#include "Misc/FileHelper.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "RHI.h"
#include "EditorFramework/AssetImportData.h"
#include "HAL/UnrealMemory.h"

#define STB_IMAGE_IMPLEMENTATION
// #define STBI_MALLOC, STBI_REALLOC, and STBI_FREE
#define STBI_MALLOC(sz) FMemory::Malloc(sz,sizeof(sz))
#define STBI_FREE(ptr) FMemory::Free(ptr)
#define STBI_REALLOC(ptr,newsz) FMemory::Realloc(ptr,newsz,sizeof(newsz))
#include "stb_image.h"


namespace MineAssetCreateHelperInternal
{
#define SWAP_RG
    class FMineTextureAssetCreateHelper
    {
    public:
        static UTexture2D *CreateTexture (const FString &LongPicturePath, const FString &LongPackageName)
        {
            // Get ImageName from LongPicturePath
            FString ImageName, ImageDirPath, ImageExtName;

            FPaths::Split (LongPicturePath, ImageDirPath, ImageName, ImageExtName);
            if (!FPlatformFileManager::Get ().GetPlatformFile ().FileExists (*LongPicturePath)) {
                UE_LOG (LogMineCustomToolEditor, Error, TEXT ("Can't find image file : %s"), *LongPicturePath)
                    return nullptr;
            }

            // Read Local Image to Texture
            FPixelFormatInfo const LPixelFormat = GPixelFormats[PF_B8G8R8A8];
            TArray<uint8> RawFileData;
            bool bUseStbLib = false;
            int SizeX = 0, SizeY = 0, ImgChannels = 0;

            // Use Stb lib to load Tga file
            if (ImageExtName.Equals (TEXT ("tga"), ESearchCase::IgnoreCase))
                bUseStbLib = true;

            if (bUseStbLib) {
                RawFileData.Empty ();
                TSharedPtr<unsigned char> const StbImgDataPtr =
                    MakeShareable (stbi_load (TCHAR_TO_ANSI (*LongPicturePath), &SizeX, &SizeY, &ImgChannels, LPixelFormat.BlockBytes));
                if (StbImgDataPtr.IsValid ()) {
                    
                    //TArray<uint8> TempArray;
                    //TempArray.Empty ();
                    //TempArray.Append (StbImgDataPtr.Get (), SizeX * SizeY * LPixelFormat.BlockBytes);
                    //for (int i=0;i<SizeY;++i)
                    //{
                    //    for (int j=0; j < SizeY; ++j)
                    //    {
                    //        int const Index = j + SizeY * i;
                    //        for (int Count=0;Count < LPixelFormat.BlockBytes;++Count)
                    //        {
                    //            uint8 CurrentColor;
                    //            if (Count >= ImgChannels)
                    //            {
                    //                CurrentColor = TempArray[Index + ImgChannels];
                    //            }else
                    //            {
                    //                CurrentColor = TempArray[Index + Count];
                    //            }
                    //            RawFileData.Emplace (CurrentColor);
                    //        }
                    //    }
                    //}

                    RawFileData.Append ((uint8*)(StbImgDataPtr.Get ()), SizeX * SizeY * LPixelFormat.BlockBytes);
                    bUseStbLib = true;
                    // stbi_image_free (StbImgDataPtr.Get ());
                }
                else return nullptr;
            }
            else if (!FFileHelper::LoadFileToArray (RawFileData, *LongPicturePath)) {
                UE_LOG (LogMineCustomToolEditor, Error, TEXT ("Can't read image file : %s"), *LongPicturePath)
                    return nullptr;
            }


            // Reformat Pixel Data
            if (RawFileData.Num () > 0) {
                // Make Texture2D
                UTexture2D *Texture = nullptr;
                TArray<uint8> UncompressedRGBA;

                if (!bUseStbLib) // Use unreal ImageWrapper
                {
                    IImageWrapperModule &ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule> ("ImageWrapper");
                    EImageFormat const ImgInputFormat =
                        ImageWrapperModule.DetectImageFormat (RawFileData.GetData (), RawFileData.Num ());
                    TSharedPtr<IImageWrapper> const ImageWrapper = ImageWrapperModule.CreateImageWrapper (ImgInputFormat);

                    if (ImageWrapper.IsValid ()) {
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
#ifdef SWAP_RG
                    bool bSaved = true;
                    for (int Index = 0; Index < RawFileData.Num (); ++Index) {
                        if (Index % 2 == 0 && Index >=2) {
                            if (!bSaved) {
                                bSaved = true;
                                continue;
                            }
                            if (bSaved) {
                                RawFileData.Swap (Index, Index-2);
                                bSaved = false;
                            }
                        }
                    }// End For loop
#endif

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
                Texture->AssetImportData->AddFileName (LongPicturePath, 0);

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
                    SAVE_Async | SAVE_NoError
                );
                return Texture;

            }
            else {
                UE_LOG (LogMineCustomToolEditor, Error, TEXT ("NoData has been loaded"));
                return nullptr;
            }

        }

        static UTexture2D *CreateTexture (UObject *OuterPackage, const TArray<uint8> &PixelData, int32 &InSizeX, int32 &InSizeY, const FPixelFormatInfo &InPixelFormat, const FName &TextureName)
        {
            // Shamelessly copied from UTexture2D::CreateTransient with a few modifications

            UTexture2D *NewTexture = NewObject<UTexture2D> (OuterPackage, TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
            NewTexture->AddToRoot ();
            FTexturePlatformData *const LPlatformData = new FTexturePlatformData ();
            NewTexture->PlatformData = LPlatformData;
            NewTexture->PlatformData->SetNumSlices (1);
            NewTexture->PlatformData->SizeX = InSizeX;
            NewTexture->PlatformData->SizeY = InSizeY;
            NewTexture->PlatformData->PixelFormat = InPixelFormat.UnrealFormat;

            //// Determine whether it is a power of 2 to use mipmap
            if ((InSizeX & (InSizeX - 1) || (InSizeY & (InSizeY - 1))))
                NewTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

            uint32 const PixelSize = InSizeX * InSizeY * InPixelFormat.BlockBytes;

            // Allocate first mipmap. (Mipmap0)
            FTexture2DMipMap *Mip = new FTexture2DMipMap ();
            NewTexture->PlatformData->Mips.Add (Mip);
            Mip->SizeX = InSizeX;
            Mip->SizeY = InSizeY;

            // Lock the texture so that it can be modified
            Mip->BulkData.Lock (LOCK_READ_WRITE);
            uint8 *TextureDataPtr = static_cast<uint8 *>(Mip->BulkData.Realloc (PixelSize));
            FMemory::Memcpy (TextureDataPtr, PixelData.GetData (), PixelSize); // copy to mip0
            Mip->BulkData.Unlock ();
            
            // To initialize the data in a non-transient field 
            //NewTexture->Source.Init (InSizeX, InSizeY,
            //    1, 1,
            //    ETextureSourceFormat::TSF_BGRA8, PixelData.GetData ()
            //);
            NewTexture->UpdateResource ();
            return NewTexture;
        }
    };
}
