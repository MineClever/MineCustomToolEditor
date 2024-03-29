﻿#include <AssetMenuTools/FSkeletalMeshActionListener.h>
#include <AssetMenuTools/TMineContentBrowserExtensions_SelectedAssets_Base.h>

#include "ClothingAssetBase.h"

#include "AssetCreateHelper/FMinePackageToObjectHelper.hpp"
#include "PackageTools.h"
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include "AssetMenuTools/TAssetsProcessorFormSelection.hpp"
#include "Rendering/SkeletalMeshRenderData.h"
#include "GeometryCache.h"
#include "GeometryCacheTrack.h"
#include "ConfigIO/ConfigIO.h"

#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "FSkeletalMeshActionsListener"


/* Processor implementation */
namespace FSkeletalMeshProcessor_AutoSet_Internal
{
    using namespace MineFormatStringInternal;

    class FDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
    {
    protected:
        virtual bool Visit (const TCHAR *FilenameOrDirectory, bool bIsDirectory) override
        {
            if (bIsDirectory) {
                FString TempPath;
                // FPackageName::TryConvertFilenameToLongPackageName (FString (FilenameOrDirectory), TempPath);
                DirectoryArray.AddUnique(FString (FilenameOrDirectory));
            }
            return true;
        }
    public:
        TArray<FString> DirectoryArray;
    };

    class FSkeletalMeshProcessor_AutoSet : public TAssetsProcessorFormSelection_Builder<LocAssetType>
    {
    public:
        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {
            TArray<UObject *> ObjectToSave;
            for (auto SkIt = Assets.CreateConstIterator (); SkIt; ++SkIt) {
                auto const SkMesh = *SkIt;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *SkMesh->GetPathName ());
                SkMesh->Modify ();
                // Do jobs
                for (int32 LodId=0; LodId < SkMesh->GetLODInfoArray().Num();++LodId)
                {
                    FSkeletalMeshLODInfo* const LODInfoPtr = SkMesh->GetLODInfo(LodId);
                    if (LODInfoPtr) {
                        FSkeletalMeshBuildSettings BuildOptions = LODInfoPtr->BuildSettings;

                        //Adjacency buffer, full precision UV and High precision tangent cannot be change in the re-import options, it must not be change from the original data.
                        BuildOptions.bBuildAdjacencyBuffer = true;
                        BuildOptions.bUseFullPrecisionUVs = true;
                        BuildOptions.bUseHighPrecisionTangentBasis = true;

                        BuildOptions.ThresholdUV = 0.000001f;
                        BuildOptions.ThresholdPosition = 0.000001f;
                        BuildOptions.ThresholdTangentNormal = 0.000001f;
                        BuildOptions.MorphThresholdPosition = 0.0001f;

                        //A degenerate face is a face with two vertex indices that are identical, and therefore no geometric area, and no normal .
                        //Some applications consider degenerate faces illegal, and do not allow them to be created or imported.
                        BuildOptions.bRemoveDegenerates = true;

                        BuildOptions.bUseMikkTSpace = false;
                        BuildOptions.bRecomputeNormals = false;
                        BuildOptions.bRecomputeTangents = false;

                        //Copy all the build option to reflect any change in the setting using the re-import UI
                        LODInfoPtr->BuildSettings = BuildOptions;
                    }
                }

                SkMesh->Build ();
                ObjectToSave.Add (SkMesh);
            }
            UPackageTools::SavePackagesForObjects (ObjectToSave);
        }
    };

    /**
     * @brief 
     */
    class FSkeletalMeshProcessor_AbcClothBindToMatSlots : public TAssetsProcessorFormSelection_Builder<LocAssetType>
    {

    public:
        FSkeletalMeshProcessor_AbcClothBindToMatSlots():TAssetsProcessorFormSelection_Builder<LocAssetType>(false)
        {
            // Without SourceControl init
        }

        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {

            TArray<UObject *> ObjectToSave;
            bool const IsSourceControlValid = FAssetSourceControlHelper::IsSourceControlAvailable ();

            for (auto MeshIterator = Assets.CreateConstIterator (); MeshIterator; ++MeshIterator) {

                // Start job!
                auto const CurrentMesh = *MeshIterator;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *CurrentMesh->GetPathName ());

                // Generate abc dir path
                TArray<FString> AbcPathArray;
                MakeRelativeAbcDirPath (CurrentMesh->GetPathName(), AbcPathArray);

                // Found Materials
                TArray<FSkeletalMaterial> AllMats = CurrentMesh->GetMaterials ();

                // Find Mat by MatIndex
                for (uint16 MatId = 0; MatId < AllMats.Num (); ++MatId) {
                    FName CurMatSlotName = AllMats[MatId].MaterialSlotName;
                    // Traversal all Alembic Sub-Path
                    for (auto AbcDirPath : AbcPathArray) {
                        // UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Searching @ %s"), *AbcDirPath);
                        if (!FPaths::DirectoryExists (AbcDirPath)) continue;

                        FString MatchedPackagePath;
                        // Make sure SlotNamed Abc Package path is valid, Or Skip
                        if (!HasFoundClothAbcFile (CurMatSlotName, AbcDirPath, MatchedPackagePath))
                            continue;

                        // Load Asset to UObject to modify
                        UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Found Matched Package @ %s"), *MatchedPackagePath);
                        UObject *const AbcAsset = MinePackageLoadHelper::LoadAsset (MatchedPackagePath);
                        if (! IsValid(AbcAsset)) continue;
                        

                        // Check if abc type
                        auto const GeoCache = Cast<UGeometryCache> (AbcAsset);
                        if (GeoCache == nullptr || GeoCache->StaticClass ()->GetFName () != UGeometryCache::StaticClass ()->GetFName ()) {
                            UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s is not valid ABC GeometryCache, SKIP\n"), *AbcAsset->GetFullName ());
                            continue;
                        }

                        // Get Current Geometry Materials
                        TArray<UMaterialInterface *> GeoCacheMatArray = GeoCache->Materials;

                        // Check if valid Mat on GeometryCache
                        if (GeoCacheMatArray.Num () > 0) {
                            // Check if Already Material has been set
                            if (GeoCacheMatArray[0]->GetPathName () == AllMats[MatId].MaterialInterface->GetPathName ())
                                continue;
                        }
                        else continue;

                        // Check Out file to Modify
                        if (IsSourceControlValid) {
                            if (!FAssetSourceControlHelper::CheckOutFile (MatchedPackagePath)) {
                                UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s Can't checkout, SKIP\n"), *AbcAsset->GetFullName ());
                                continue;
                            }
                        }

                        // Replace Current Mat
                        GeoCache->Modify ();
                        for (uint16 GeoMatId = 0; GeoMatId < GeoCacheMatArray.Num (); ++GeoMatId) {
                            GeoCacheMatArray[GeoMatId] = AllMats[MatId].MaterialInterface;
                        }

                        // Update GeometryCache Materials
                        GeoCache->Materials = GeoCacheMatArray;
                        ObjectToSave.Add (AbcAsset);
                    } // Terminate Traversal AbcPathArray
                } // End Of Iterator of MatIds
            } // End of Iterator Of Assets
            UPackageTools::SavePackagesForObjects (ObjectToSave);
        } // End Of ProcessAssets

        /**
         * @brief :Check if Slot name based Abc File in inputted Abc Directory Path
         * @param MatSlotName :FName
         * @param AbcDirPath :FString
         * @param MatchedPackagePath :FString
         * @param MatchedObjectPath :FString
         * @return : if Matched Abc StaticMesh Name
         */
        static bool HasFoundClothAbcFile (const FName & MatSlotName,const FString & AbcDirPath, FString & MatchedPackagePath)
        {
            auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
            FString const ConfigSubPathRule =
                ConfigSettings->bUseCustomProxyConfig ?
                ConfigSettings->ConfigAlembicClothSubDirMatchKey : TEXT ("Cloth");
            MatchedPackagePath = FPaths::ConvertRelativePathToFull (AbcDirPath / ConfigSubPathRule, MatSlotName.ToString ());
            FPackageName::TryConvertFilenameToLongPackageName (MatchedPackagePath, MatchedPackagePath);
            // UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Try to found Alembic @ %s"), *MatchedPackagePath);
            if (FPackageName::DoesPackageExist (MatchedPackagePath)) {
                return true;
            }

            return false;
        }

        static void MakeRelativeAbcDirPath (const FString & MatPackagePath, TArray<FString> &AbcPathArray)
        {
            AbcPathArray.Empty ();
            auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
            FString const ConfigAlembicPathRule =
                ConfigSettings->bUseCustomProxyConfig ?
                ConfigSettings->ConfigAlembicPathRule :
                TEXT ("Animations/Alembic");

            // Path @ [CurrentAssetDir]/Animations/Alembic
            FString TempDirPath;
            FPackageName::TryConvertLongPackageNameToFilename (MatPackagePath, TempDirPath);
            TempDirPath = FPaths::GetPath (TempDirPath) / ConfigAlembicPathRule;
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to found all Alembic Diectory @ %s"),*TempDirPath);
            if (FPaths::DirectoryExists(TempDirPath))
            {
                FDirectoryVisitor Visitor;
                FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*TempDirPath, Visitor);
                AbcPathArray = Visitor.DirectoryArray;
            }
        }
    }; // End Of Class

    class FSkeletalMeshProcessor_AbcTrackMatBindToMatSlots : public TAssetsProcessorFormSelection_Builder<LocAssetType>
    {
    private:
        bool &&bDebugCls = false;

    public:
        FSkeletalMeshProcessor_AbcTrackMatBindToMatSlots () :TAssetsProcessorFormSelection_Builder<LocAssetType> (false)
        {
            // Without SourceControl init
        }

        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {

            TArray<UObject *> ObjectToSave;
            bool const IsSourceControlValid = FAssetSourceControlHelper::IsSourceControlAvailable ();

            auto LambdaCheckIfSameMat = [&](const FString &CurrentMatPath, const TArray<FSkeletalMaterial> &AllMats)->bool {
                bool bHasFoundSamePath = false;
                // Find Mat by MatIndex
                for (uint16 MatId = 0; MatId < AllMats.Num (); ++MatId) {
                    bHasFoundSamePath = AllMats[MatId].MaterialInterface->GetPathName () == CurrentMatPath;
                    if (bHasFoundSamePath)
                    {
                        if (bDebugCls) UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Input : %s ; Mat Matched @ MatID %d ;Same as current setted one\n"), *CurrentMatPath, MatId);
                        break;
                    }
                }
                return bHasFoundSamePath;
            };

            // NOTE: Use regex to search main ruled name && material section index in cache
            // "^.*?_(.*?)\\d?(?:Shape)_(\\d?)$" match "Abc_GreenOneShape_0" <- Alembic Stream-able Track Name
            // Get $1 == "GreenOne", $2 == "0"
            auto LambdaRegexMatchShape = [&](const FString &Str, TArray<FString> &Result)->bool {
                // TODO: Should fix use string finder
                static const FRegexPattern Patten = FRegexPattern(TEXT ("^(?:.*?_)?(.*?)(?:_\\d*?)?Shape_(\\d*?)$"));
                FRegexMatcher Matcher (Patten, Str);
                Result.Empty ();
                while (Matcher.FindNext ()) {
                    Result.Emplace (Matcher.GetCaptureGroup (1));
                    Result.Emplace (Matcher.GetCaptureGroup (2));
                    if (bDebugCls) UE_LOG (LogMineCustomToolEditor, Log, TEXT ("MatChecker Input : %s ; Regex Matched :%s @ %s \n"), *Str, *Result[0], *Result[1]);
                }
                return Result.Num () == 0 ? false : true;
            };

            auto LambdaFindMatIdByName = [&](const FString &NameString, const TArray<FSkeletalMaterial> &AllMats, uint16 &RefMatID) -> bool
            {
                bool bHasFoundMatchedMatId = false;
                if (bDebugCls) UE_LOG (LogMineCustomToolEditor, Log, TEXT ("MatFinder Try to find with key name : %s ;\n"), *NameString);

                TArray<FString> IgnoreKeywords = { TEXT("_DaiLi"), TEXT("_Proxy")};

                for (uint16 MatId = 0; MatId < AllMats.Num (); ++MatId) {

                    /* NOTE: Ready for process with names */
                    if (!AllMats[MatId].MaterialInterface->IsValidLowLevel())
                        continue;

                    FString &&CurMatInterfaceName = AllMats[MatId].MaterialInterface->GetName();
                    FString &&CurMatSlotName = AllMats[MatId].MaterialSlotName.ToString ();
                    UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Try to find with interface name :%s, matslot name :%s\n"), *CurMatInterfaceName, *CurMatSlotName);


                    /* NOTE: Skip with IgnoreKeywords */
                    bool &&bSkipWithIgnoreKeyword = false;
                    for (const FString Keyword : IgnoreKeywords)
                    {
                        if (CurMatInterfaceName.EndsWith (Keyword) || CurMatSlotName.EndsWith (Keyword)) {
                            UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Name [%s] with Proxy name, skip! \n"), *NameString);
                            bSkipWithIgnoreKeyword = true;
                            break;
                        }
                    }
                    if ((bSkipWithIgnoreKeyword)) continue;


                    /* NOTE: Check with current Material Interface name */
                    if (CurMatInterfaceName.Find (NameString,ESearchCase::IgnoreCase,ESearchDir::FromEnd) > 0)
                    {

                        UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Mat Interface Mode; Mat Matched @ MatID %d \n"), *NameString, MatId);
                        RefMatID = MatId;
                        bHasFoundMatchedMatId = bHasFoundMatchedMatId || true;
                    }
                    /* NOTE: Check with current Material Slot name */
                    if (!bHasFoundMatchedMatId)
                    {
                        UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Not found by Mat Interface Mode , using Mat SlotName Mode\n"));
                        if (CurMatSlotName.Find (NameString, ESearchCase::IgnoreCase, ESearchDir::FromEnd) < 0) continue;
                        else
                        {
                            UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Mat SlotName Mode; Mat Matched @ MatID %d \n"), MatId);
                            RefMatID = MatId;
                            bHasFoundMatchedMatId = bHasFoundMatchedMatId || true;
                        }
                    }
                }
                return bHasFoundMatchedMatId;
            };

            for (auto const CurrentMesh : Assets) {

                // Start job!
                auto const CurrentPathName = CurrentMesh->GetPathName ();
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *CurrentPathName);

                // Generate abc dir path
                TArray<FString> AbcPathArray;
                MakeRelativeAbcDirPath (CurrentPathName, AbcPathArray);

                // Found Materials
                TArray<FSkeletalMaterial> AllMats = CurrentMesh->GetMaterials ();


                // Traversal all Alembic Sub-Path
                for (auto AbcDirPath : AbcPathArray) {

                    if (!FPaths::DirectoryExists (AbcDirPath)) continue;

                    // Make sure Abc Package path is valid, Or Skip
                    TArray<FString> MatchedPackagePaths;
                    if (!HasFoundAnimationAbcFiles (AbcDirPath, MatchedPackagePaths))
                        continue;

                    for (auto const MatchedPackagePath: MatchedPackagePaths)
                    {
                        // Load Asset to UObject to modify
                        UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Found Matched Package @ %s"), *MatchedPackagePath);
                        UObject *const AbcAsset = MinePackageLoadHelper::LoadAsset (MatchedPackagePath);
                        if (!IsValid (AbcAsset)) continue;

                        // Check if abc type
                        auto const GeoCache = Cast<UGeometryCache> (AbcAsset);
                        if (GeoCache == nullptr || GeoCache->StaticClass ()->GetFName () != UGeometryCache::StaticClass ()->GetFName ()) {
                            UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s is not valid ABC GeometryCache, SKIP\n"), *AbcAsset->GetFullName ());
                            continue;
                        }

                        // NOTE: Get Current Geometry Materials && Tracks
                        TArray<UMaterialInterface *> GeoCacheMatArray = GeoCache->Materials;
                        TArray<UGeometryCacheTrack *> GeoCacheTracks = GeoCache->Tracks;
                        int32 &&GeometryCacheTracksCount = GeoCacheTracks.Num ();
                        auto &&GeoCacheMatCount = GeoCacheMatArray.Num ();

                        // NOTE: Skip when Flattened track or no track here
                        if (GeometryCacheTracksCount < 1 || GeoCacheTracks[0]->GetFName() == FName(TEXT("Flattened_Track")))
                        {
                            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s is Flattened Alembic Track, SKIP!\n"), *AbcAsset->GetFullName ());
                            continue;
                        }

                        bool bShouldModify = false;
                        for (uint16 GeoCacheTrackId=0; GeoCacheTrackId < GeometryCacheTracksCount; ++GeoCacheTrackId)
                        {
                            FString CurrentTrackName = GeoCacheTracks[GeoCacheTrackId]->GetName();
                            uint16 MatchedCurMatId = 0;
                            FString MatchedCurMainName = "UnknownMesh";

                            // NOTE: Found main mesh name and section index
                            static TArray<FString> RegexMatchResult;
                            if (LambdaRegexMatchShape (CurrentTrackName, RegexMatchResult))
                            {
                                MatchedCurMainName = RegexMatchResult[0];
                                // MatchedCurMatId = FCString::Atoi (*RegexMatchResult[1]);
                                MatchedCurMatId = GeoCacheTrackId > GeoCacheMatCount ? GeoCacheMatCount : GeoCacheTrackId;
                            } else continue;


                            // NOTE: Use main string to make slot mat name
                            uint16 RefMeshMatId;
                            if (!LambdaFindMatIdByName (MatchedCurMainName, AllMats, RefMeshMatId))
                                continue;

                            // NOTE: Check if Already Material has been set
                            //if (LambdaCheckIfSameMat (GeoCacheMatArray[MatchedCurMatId]->GetPathName (), AllMats))
                            //    continue;
                            if (GeoCacheMatArray[MatchedCurMatId]->GetPathName () == AllMats[RefMeshMatId].MaterialInterface->GetPathName())
                                continue;


                            GeoCacheMatArray[MatchedCurMatId] = AllMats[RefMeshMatId].MaterialInterface;
                            bShouldModify = true;
                        }
                        if (!bShouldModify) continue;

                        // Check Out file to Modify
                        if (IsSourceControlValid) {
                            if (!FAssetSourceControlHelper::CheckOutFile (MatchedPackagePath)) {
                                UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s Can't checkout, SKIP\n"), *AbcAsset->GetFullName ());
                                continue;
                            }
                        }

                        // Update GeometryCache Materials
                        GeoCache->Modify ();
                        GeoCache->Materials = GeoCacheMatArray;
                        ObjectToSave.Add (AbcAsset);
                    }
                }
            } 
            UPackageTools::SavePackagesForObjects (ObjectToSave);
        } // End Of ProcessAssets

        /**
         * @brief :Check if Animation Cache Abc File in inputted Abc Directory Path with config rule
         */
        static bool HasFoundAnimationAbcFiles (const FString &AbcDirPath, TArray<FString> &MatchedPackagePaths)
        {

            // Read config to match sub-dir
            auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
            const FString &&ConfigSubPathRule =
                ConfigSettings->bUseCustomProxyConfig ?
                ConfigSettings->ConfigAlembicAnimationSubDirMatchKey : TEXT ("AnimCache");
            FString &&MatchedDirPath = FPaths::ConvertRelativePathToFull(AbcDirPath / ConfigSubPathRule);

            // Find all package name under current AnimCache Directory
            MatchedPackagePaths.Empty ();
            IFileManager::Get ().FindFiles (MatchedPackagePaths, *MatchedDirPath, TEXT ("uasset"));

            // NOTE: make fully path
            for (int PathId =0 ;PathId < MatchedPackagePaths.Num (); ++PathId)
            {
                //PackageName = FPaths::ConvertRelativePathToFull (MatchedDirPath, PackageName);
                 FPackageName::TryConvertFilenameToLongPackageName ((MatchedDirPath / MatchedPackagePaths[PathId]), MatchedPackagePaths[PathId]);
            }

            return MatchedPackagePaths.Num () > 0 ? true : false;
        }

        static void MakeRelativeAbcDirPath (const FString &MatPackagePath, TArray<FString> &AbcPathArray)
        {
            AbcPathArray.Empty ();
            auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
            FString const ConfigAlembicPathRule =
                ConfigSettings->bUseCustomProxyConfig ?
                ConfigSettings->ConfigAlembicAnimCachePathRule :
                TEXT ("Animations/Alembic");

            // Path @ [CurrentAssetDir]/Animations/Alembic
            FString TempDirPath;
            FPackageName::TryConvertLongPackageNameToFilename (MatPackagePath, TempDirPath);
            TempDirPath = FPaths::GetPath (TempDirPath) / ConfigAlembicPathRule;
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to found all Alembic Diectory @ %s"), *TempDirPath);
            if (FPaths::DirectoryExists (TempDirPath)) {
                FDirectoryVisitor Visitor;
                FPlatformFileManager::Get ().GetPlatformFile ().IterateDirectory (*TempDirPath, Visitor);
                AbcPathArray = Visitor.DirectoryArray;
            }
        }

    }; // End Of Class

    class FSkeletalMeshProcessor_AutoBindMaterials: public TAssetsProcessorFormSelection_Builder<LocAssetType>
    {

    public:

        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {

            TArray<UObject *> ObjectToSave;

            for (auto SkIt = Assets.CreateConstIterator (); SkIt; ++SkIt) {

                // Start job!
                auto const SkMesh = *SkIt;

                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *SkMesh->GetPathName ());

                // Generate abc dir path
                TArray<FString> MatDirPathArray;
                MineMaterialPackageHelper::MakeRelativeMatDirPath (SkMesh->GetPathName (), MatDirPathArray);

                // Found Materials
                TArray<FSkeletalMaterial> AllMats = SkMesh->GetMaterials ();

                // Find Mat by MatIndex
                FString MatchedPackagePath;
                for (uint16 MatId = 0; MatId < AllMats.Num (); ++MatId) {
                    FName CurMatSlotName = AllMats[MatId].MaterialSlotName;

                    // traverse all valid path to find same named material
                    for (FString MatDirPath: MatDirPathArray)
                    {
                        if (!MineMaterialPackageHelper::HasFoundSlotNameMat(CurMatSlotName, MatDirPath, MatchedPackagePath)) continue;

                        // Load Material to set
                        UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Found Matched Package @ %s"), *MatchedPackagePath);
                        UObject *const MatAsset = MinePackageLoadHelper::LoadAsset (MatchedPackagePath);
                        if (!IsValid (MatAsset)) continue;

                        // Cast to material type && Set new Material Interface
                        UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Type as @ %s"), *MatAsset->GetClass ()->GetName ());
                        auto const CurMatInterface = Cast<UMaterialInterface> (MatAsset);
                        if (CurMatInterface == nullptr) continue;
                        AllMats[MatId].MaterialInterface = CurMatInterface;
                        break;
                    }
                } // End Of Iterator of MatIds

                SkMesh->Modify ();
                SkMesh->SetMaterials (AllMats);
                ObjectToSave.Emplace (SkMesh);

            } // End of Iterator Of Assets
            UPackageTools::SavePackagesForObjects (ObjectToSave);
        } // End Of ProcessAssets

        void QuerySubDirAssets (TArray<FString> &MatDirPathArray)
        {
            // NOTE: Return, If not valid FirstLayer Material Dir
            if (!MatDirPathArray.IsValidIndex (0)) return;
            auto MatDirPathArrayCopy = TArray<FString> (MatDirPathArray); // NOTE: Make a copy

            auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
            FString const ConfigMatPathRule =
                ConfigSettings->bUseCustomMaterialBindConfig ?
                ConfigSettings->ConfigMaterialDirectoryRule :
                TEXT ("material,mat,materials");

            // NOTE: For each sub dir in current first layer dirs, if it is valid.
            for (auto CurMatDir : MatDirPathArrayCopy)
            {

            }

        }

    }; // End Of Class

    class FSkeletalMeshProcessor_AutoBindClothData : public TAssetsProcessorFormSelection_Builder<LocAssetType>
    {
        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {
            TArray<UObject *> ObjectToSave;
            
            for (LocAssetType* const SkMesh : Assets)
            {
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *SkMesh->GetPathName ());


                // Found Materials
                TArray<FSkeletalMaterial> AllMats = SkMesh->GetMaterials ();

                // Found ClothData
                TArray<UClothingAssetBase*> AllClothData = SkMesh->GetMeshClothingAssets();
                

                SkMesh->Modify ();
                for (uint16 LodId=0;LodId< SkMesh->GetLODNum ();++LodId)
                {

                    // Find by Section
                    TIndirectArray<FSkeletalMeshLODRenderData> &LodRenderData = SkMesh->GetResourceForRendering ()->LODRenderData;
                    uint16 SectionsNum = 0;

                    // Make sure that the Sections is valid!
                    if (LodRenderData.IsValidIndex (LodId)) {
                        if (!LodRenderData[LodId].HasClothData ()) continue;
                        SectionsNum = LodRenderData[LodId].RenderSections.Num ();
                    }
                    else continue;

                    for (int SectionId = 0; SectionId < SectionsNum; ++SectionId) 
                    {
                        uint16 const CurSectionMatId = LodRenderData[LodId].RenderSections[SectionId].MaterialIndex;
                        FName const CurMatSlotName = AllMats[CurSectionMatId].MaterialSlotName;
                        for (uint16 ClothDataId =0; ClothDataId< AllClothData.Num(); ++ClothDataId)
                        {
                            // Check if matched material slot
                            if (AllClothData[ClothDataId]->GetFName () == CurMatSlotName)
                            {
                                // Set Current Section to Matched ClothData
                                //LodRenderData[LodId].RenderSections[SectionId].ClothingData.AssetGuid = AllClothData[ClothDataId]->GetAssetGuid ();
                                AllClothData[ClothDataId]->BindToSkeletalMesh (SkMesh,LodId,SectionId,0);
                                break;
                            }
                        }
                    }// End Traversal SectionsNum
                }
                ObjectToSave.Emplace (SkMesh);
            }
            // UPackageTools::SavePackagesForObjects (ObjectToSave);
        }
    }; // End Of Class

}

/* Command Info */
namespace FSkeletalMeshActionsMenuCommandsInfo_Internal
{
    using namespace FSkeletalMeshProcessor_AutoSet_Internal;
    using namespace TMineContentBrowserExtensions_SelectedAssets_Internal;

    class MineAssetCtxMenuCommandsInfo final : public TCommands<MineAssetCtxMenuCommandsInfo>, public MineAssetCtxMenuCommands_CommandMap
    {
    public:

        /* INIT */
        static FName MenuCtxName;
        MineAssetCtxMenuCommandsInfo ()
            : TCommands<MineAssetCtxMenuCommandsInfo> (
                MenuCtxName, // Context name for fast lookup
                FText::FromString (MenuCtxName.ToString ()), // Context name for displaying
                NAME_None,   // No parent context
                FEditorStyle::GetStyleSetName () // Icon Style Set
                )
        {
        }

        /* Register all commands in this class */
        virtual void RegisterCommands () override
        {
            // 0
            FORMAT_COMMAND_INFO (0,
                "Auto set SkMesh Prop",
                "Auto set configs for selected SkeletalMesh assets.",
                FSkeletalMeshProcessor_AutoSet
            );

            // 1
            FORMAT_COMMAND_INFO (1,
                "Set ABC-Cloth Mat",
                "Auto set Alembic GeometryCache Materials reference to selected SkeletalMesh assets.\n"
                "such that it has been followed by [CurrentAssetDir]/Animations/Alembic/[SubDir]/Cloth/[MatSlotName]",
                FSkeletalMeshProcessor_AbcClothBindToMatSlots
            );
            // 2
            FORMAT_COMMAND_INFO (2,
                "Set ABC-AnimCache Mat",
                "Auto set Alembic GeometryCache Materials reference to selected SkeletalMesh assets.\n"
                "such that it has been followed by [CurrentAssetDir]/Animations/Alembic/[SubDir]/AnimCache",
                FSkeletalMeshProcessor_AbcTrackMatBindToMatSlots
            );
            // 3
            FORMAT_COMMAND_INFO (3,
                "Set Cloth to MatSlot",
                "Auto set existed Cloth-Data with matched slot name for selected SkeletalMesh assets.",
                FSkeletalMeshProcessor_AutoBindClothData
            );

            //4
            FORMAT_COMMAND_INFO (4,
                "Auto bind Material",
                "Auto set existed Material with matched slot name for selected SkeletalMesh assets.",
                FSkeletalMeshProcessor_AutoBindMaterials
            );
            // END

        }
    };
    // Init Context Fast look up name
    FName MineAssetCtxMenuCommandsInfo::MenuCtxName = TEXT ("MineUSkeletalMeshAssetCtxMenu");

}

/* Menu Extension */
namespace  FSkeletalMeshActionsMenuExtension_Internal
{
    using namespace FSkeletalMeshActionsMenuCommandsInfo_Internal;
    using namespace TMineContentBrowserExtensions_SelectedAssets_Internal;

    /* Extension to menu */
    class FMineContentBrowserExtensions_SelectedAssets : public TMineContentBrowserExtensions_SelectedAssets_Base<LocAssetType, MineAssetCtxMenuCommandsInfo>
    {
    public:

        FMineContentBrowserExtensions_SelectedAssets ()
        {
            FMineContentBrowserExtensions_SelectedAssets::InitSubMenu ();
        }

        virtual void InitSubMenu () override
        {

            FNsLocTextDescriptions LSubMenuDescriptions;
            LSubMenuDescriptions.Key = TEXT ("SKMeshActionsSubMenuLabel");
            LSubMenuDescriptions.KeyDescription = TEXT ("Mine SkeletalMesh Asset Actions");
            LSubMenuDescriptions.LocTextNameSpace = TEXT (LOCTEXT_NAMESPACE);
            this->SubMenuDescriptions = LSubMenuDescriptions;

            FNsLocTextDescriptions LSubMenuTipDescriptions;
            LSubMenuTipDescriptions.Key = TEXT ("SKMeshActionsSubMenuToolTip");
            LSubMenuTipDescriptions.KeyDescription = TEXT ("Type-related actions for SkeletalMesh Asset.");
            LSubMenuTipDescriptions.LocTextNameSpace = TEXT (LOCTEXT_NAMESPACE);
            this->SubMenuTipDescriptions = LSubMenuTipDescriptions;

            this->SubMenuInExtensionHookName = FName (TEXT ("MineSkeletalMeshAssetsActions"));
            this->SubMenuIcon = FSlateIcon ();
        }

    };
};


/* Load to module */
namespace FSkeletalMeshMenuActionsListener_Internal
{
    using namespace FSkeletalMeshActionsMenuExtension_Internal;

    // Important : Init Extension Class
    TSharedRef<TMineContentBrowserExtensions_SelectedAssets_Base<LocAssetType, MineAssetCtxMenuCommandsInfo>>
        FSkeletalMeshMenuActionsListener::MenuExtension = 
            MakeShareable (new FMineContentBrowserExtensions_SelectedAssets);

    void FSkeletalMeshMenuActionsListener::InstallHooks ()
    {
        // Push Log
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install %s Asset Menu Hook"), *FMineContentBrowserExtensions_SelectedAssets::AssetTypeName.ToString());

        // register commands
        MineAssetCtxMenuCommandsInfo::Register ();

        // Declare Delegate 
        ContentBrowserExtenderDelegate =
            FContentBrowserMenuExtender_SelectedAssets::CreateSP (
                MenuExtension,
                &FMineContentBrowserExtensions_SelectedAssets::OnExtendContentBrowserAssetSelectionMenu
            );

        // Get all content module delegates
        TArray<FContentBrowserMenuExtender_SelectedAssets> &CBMenuExtenderDelegates = GetExtenderDelegates ();

        // Add The delegate of mine
        CBMenuExtenderDelegates.Add (ContentBrowserExtenderDelegate);

        // Store handle
        ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last ().GetHandle ();
    }
}


#undef LOCTEXT_NAMESPACE