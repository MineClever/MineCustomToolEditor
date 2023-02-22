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
#include "PackageTools.h"



namespace MineAssetCreateHelperInternal
{

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(sz) FMemory::Malloc(sz,sizeof(sz))
#define STBI_FREE(ptr) FMemory::Free(ptr)
#define STBI_REALLOC(ptr,newsz) FMemory::Realloc(ptr,newsz,sizeof(newsz))
#include "stb_image.h"

    class FMineTextureAssetCreateHelper
    {
    public:
        static void CreateTextureFromVirtualTexture (UTexture2D* const VirtualTexObj)
        {
            UPackage * CurrentPackage = VirtualTexObj->GetPackage();
            FString &&CurrentVT_PackageLongName = CurrentPackage->GetName ();

            // Set Texture as nice map
            VirtualTexObj->SRGB = false;
            VirtualTexObj->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
            VirtualTexObj->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
            VirtualTexObj->UpdateResource ();

            // Read image data from VirtualTexture
            const FTexturePlatformData* const PlatformData = VirtualTexObj->GetPlatformData();
            int32 const VT_WidthInTilesCount = PlatformData->VTData->GetWidthInTiles ();
            int32 const VT_HeightInTilesCount = PlatformData->VTData->GetHeightInTiles();
            int32 &&VT_TitleSize = PlatformData->VTData->TileSize;

            auto VT_DataChunks = PlatformData->VTData->Chunks;
            auto CreateAndSaveTexPackage =
                [&](const FString &CurrentChunkOuterPackageName, const uint8* &TextureDataRawArray) {
                UPackage *TexturePackage = CreatePackage (*CurrentChunkOuterPackageName);
                TexturePackage->FullyLoad ();

                FName const ShortTextureName = *FPaths::GetBaseFilename (CurrentChunkOuterPackageName);
                UTexture2D *Texture = CreateTextureFromPixelData (TexturePackage, TextureDataRawArray, VT_TitleSize,
                    VT_TitleSize, GPixelFormats[PF_B8G8R8A8],
                    ShortTextureName);

                // Mark package dirty
                TexturePackage->MarkPackageDirty ();

                // Register Asset
                FAssetRegistryModule::AssetCreated (Texture);

                // Get Package File Name
                FString &&PackageFileName = FPackageName::LongPackageNameToFilename (CurrentChunkOuterPackageName);

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
            };

            // Find Color in each pixel
            for (int32 TileHeightIndex = 0;TileHeightIndex < VT_HeightInTilesCount;++TileHeightIndex)
            {
                int32 LTile_Udim_Index = (TileHeightIndex + 1 )* 1000;
                for (int32 TileWidthIndex = 0;TileWidthIndex < VT_WidthInTilesCount;++TileWidthIndex)
                {
                    ++LTile_Udim_Index;
                    int32 const VT_TitleIndex = TileWidthIndex + TileHeightIndex;
                    int32 const ChunkIndex = PlatformData->VTData->GetChunkIndex (VT_TitleIndex);
                    const FVirtualTextureDataChunk &CurrentChunkData = VT_DataChunks[ChunkIndex];

                    // Read TextureData from current VT chunk
                    const uint8 *TextureDataRawArray= 
                        static_cast <const uint8 *>(CurrentChunkData.BulkData.LockReadOnly ());
                    CurrentChunkData.BulkData.Unlock ();

                    // Create New Package Name From Current Chunk
                    FString CurrentChunkOuterPackageName = 
                        CurrentVT_PackageLongName + TEXT ("_") + FString::FromInt (LTile_Udim_Index);
                    CreateAndSaveTexPackage (CurrentChunkOuterPackageName, TextureDataRawArray);
                }
            }

            // Reload Package to reset change
            TArray<UPackage *> AssetPackages;
            AssetPackages.AddUnique (CurrentPackage);
            UPackageTools::ReloadPackages (AssetPackages);
        }

        static UTexture2D *CreateTexture (const FString &LongPicturePath, const FString &LongPackageName)
        {
            // Get ImageName from LongPicturePath
            FString ImageName, ImageDirPath, ImageExtName;

            FPaths::Split (LongPicturePath, ImageDirPath, ImageName, ImageExtName);
            ImageName = ImageName.Replace(TEXT("."), TEXT("_"));
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
                    RawFileData.Append (StbImgDataPtr.Get (), SizeX * SizeY * LPixelFormat.BlockBytes);
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
            if (RawFileData.Num () == 0) {
                UE_LOG (LogMineCustomToolEditor, Error, TEXT ("No Data has been loaded"));
                return nullptr;
            }

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
                }
                else
                {
                    UE_LOG (LogMineCustomToolEditor, Error, TEXT ("Can't read format : %s"), *ImageExtName);
                    return nullptr;
                }
            } // End of (!bStbLib)
            else {
                // Swap RGBA-> BGRA;
                // push %eax
                // mov %ebx, dword ptr ss[offset] ?
                for (int Index = 0; Index < RawFileData.Num (); Index += 4) {
                    RawFileData.Swap (Index, Index + 2);
                }

                UncompressedRGBA = RawFileData;
            }

            // Make Sure Valid parameters
            if (SizeX <= 0 || SizeY <= 0 ||
                (SizeX % LPixelFormat.BlockSizeX) != 0 ||
                (SizeX % LPixelFormat.BlockSizeY) != 0) {
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Invalid parameters specified for CreateTextureFromPixelData()"));
                return nullptr;
            }

            // Create new texture pointer
            UPackage *TexturePackage = CreatePackage (*LongPackageName);
            TexturePackage->FullyLoad ();

            // Make Texture2D
            UTexture2D *Texture = nullptr;
            
            Texture = CreateTextureFromPixelData (TexturePackage, UncompressedRGBA, SizeX, SizeY, LPixelFormat, FName (*ImageName));
            auto const AssetImportDataArray = Cast<UAssetImportData>(Texture->AssetImportData);
            AssetImportDataArray->AddFileName (LongPicturePath, 0);
            // Mark package dirty
            TexturePackage->MarkPackageDirty ();

            // Register Asset
            FAssetRegistryModule::AssetCreated (Texture);

            // Get Package File Name : path/TexAsset --> path/TexAsset.uasset
            FString &&PackageFileName = FPackageName::LongPackageNameToFilename (LongPackageName);

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

        static UTexture2D *CreateTextureFromPixelData (UPackage *OuterPackage, const TArray<uint8> &PixelData, int32 &InSizeX, int32 &InSizeY, const FPixelFormatInfo &InPixelFormat, const FName &TextureName)
        {
            return CreateTextureFromPixelData (OuterPackage, PixelData.GetData (), InSizeX, InSizeY, InPixelFormat, TextureName);
        }

        static UTexture2D *CreateTextureFromPixelData (UPackage *OuterPackage, const uint8* PixelDataPtr, int32 &InSizeX, int32 &InSizeY, const FPixelFormatInfo &InPixelFormat, const FName &TextureName)
        {
            // Shamelessly copied from UTexture2D::CreateTransient with a few modifications

            UTexture2D *NewTexture = NewObject<UTexture2D> (OuterPackage, TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
            NewTexture->AddToRoot ();
            auto const LPlatformData = new FTexturePlatformData ();
            NewTexture->SetPlatformData(LPlatformData);
            NewTexture->GetPlatformData()->SetNumSlices (1);
            NewTexture->GetPlatformData()->SizeX = InSizeX;
            NewTexture->GetPlatformData()->SizeY = InSizeY;
            NewTexture->GetPlatformData()->PixelFormat = InPixelFormat.UnrealFormat;

            //// Determine whether it is a power of 2 to use mipmap
            if ((InSizeX & (InSizeX - 1) || (InSizeY & (InSizeY - 1))))
                NewTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

            uint32 const PixelSize = InSizeX * InSizeY * InPixelFormat.BlockBytes;

            // Allocate first mipmap. (Mipmap0)
            FTexture2DMipMap *Mip = new FTexture2DMipMap ();
            NewTexture->GetPlatformData()->Mips.Add (Mip);
            Mip->SizeX = InSizeX;
            Mip->SizeY = InSizeY;

            // Lock the texture so that it can be modified
            Mip->BulkData.Lock (LOCK_READ_WRITE);
            uint8 *TextureDataPtr = static_cast<uint8 *>(Mip->BulkData.Realloc (PixelSize));
            FMemory::Memcpy (TextureDataPtr, PixelDataPtr, PixelSize); // copy to mip0
            Mip->BulkData.Unlock ();
            NewTexture->UpdateResource ();

            // To initialize the data in a non-transient field (to show as * icon?)
            NewTexture->Source.Init (InSizeX, InSizeY,
                1, 1,
                ETextureSourceFormat::TSF_BGRA8, PixelDataPtr
            );
            NewTexture->UpdateResource ();
            return NewTexture;
        }
    };

}
