#include <AssetMenuTools/FSkeletalMeshActionListener.h>
#include <AssetMenuTools/TMineContentBrowserExtensions_SelectedAssets_Base.h>

#include "ClothingAssetBase.h"

#include "AssetCreateHelper/FMinePackageToObjectHelper.hpp"
#include "PackageTools.h"
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include "AssetMenuTools/TAssetsProcessorFormSelection.hpp"
#include "Rendering/SkeletalMeshRenderData.h"
#include "GeometryCache.h"
#include "ConfigIO/ConfigIO.h"

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

            for (auto SkIt = Assets.CreateConstIterator (); SkIt; ++SkIt) {

                // Start job!
                auto const SkMesh = *SkIt;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *SkMesh->GetPathName ());

                // Generate abc dir path
                TArray<FString> AbcPathArray;
                MakeRelativeAbcDirPath (SkMesh->GetPathName(), AbcPathArray);

                // Found Materials
                TArray<FSkeletalMaterial> AllMats = SkMesh->GetMaterials ();

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
                ConfigSettings->ConfigAlembicSubDirMatchKey : TEXT ("Cloth");
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
                MakeRelativeMatDirPath (SkMesh->GetPathName (), MatDirPathArray);

                // Found Materials
                TArray<FSkeletalMaterial> AllMats = SkMesh->GetMaterials ();

                // Find Mat by MatIndex
                FString MatchedPackagePath;
                for (uint16 MatId = 0; MatId < AllMats.Num (); ++MatId) {
                    FName CurMatSlotName = AllMats[MatId].MaterialSlotName;

                    // traverse all valid path to find same named material
                    for (FString MatDirPath: MatDirPathArray)
                    {
                        if (!HasFoundSlotNameMat(CurMatSlotName, MatDirPath, MatchedPackagePath)) continue;

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


        static void MakeRelativeMatDirPath (const FString &MatPackagePath, TArray<FString> &ValidPathArray)
        {
            ValidPathArray.Empty ();
            auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
            FString const ConfigMatPathRule =
                ConfigSettings->bUseCustomMaterialBindConfig ?
                ConfigSettings->ConfigMaterialDirectoryRule :
                TEXT ("material,mat,materials");

            // Convert into array
            TArray<FString> MaterialsRuleDirArray;
            ConfigMatPathRule.ParseIntoArray (MaterialsRuleDirArray, TEXT (","), true);

            for (auto MaterialRuleDir : MaterialsRuleDirArray)
            {
                FString TempDirPath;
                FPackageName::TryConvertLongPackageNameToFilename (MatPackagePath, TempDirPath);
                TempDirPath = FPaths::GetPath (TempDirPath) / MaterialRuleDir;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("TempPath as @ %s"), *TempDirPath);
                if (FPaths::DirectoryExists (TempDirPath)) {
                    ValidPathArray.AddUnique(TempDirPath);
                }
            }

        }

        static bool HasFoundSlotNameMat (const FName &MatSlotName, const FString &MatDirPath, FString &MatchedPackagePath)
        {

            MatchedPackagePath = FPaths::ConvertRelativePathToFull (MatDirPath, MatSlotName.ToString ());
            FPackageName::TryConvertFilenameToLongPackageName (MatchedPackagePath, MatchedPackagePath);
            UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Try to find Material @ %s"), *MatchedPackagePath);
            if (FPackageName::DoesPackageExist (MatchedPackagePath)) {
                return true;
            }
            return false;
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
                "Set Cloth to MatSlot",
                "Auto set existed Cloth-Data with matched slot name for selected SkeletalMesh assets.",
                FSkeletalMeshProcessor_AutoBindClothData
            );

            FORMAT_COMMAND_INFO (3,
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