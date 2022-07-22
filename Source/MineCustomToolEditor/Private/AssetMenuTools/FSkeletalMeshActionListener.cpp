#include <AssetMenuTools/FSkeletalMeshActionListener.h>
#include <AssetMenuTools/TMineContentBrowserExtensions_SelectedAssets_Base.h>

#include "FileHelpers.h"
#include "PackageTools.h"
#include "Algo/Count.h"
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include "AssetMenuTools/TAssetsProcessorFormSelection.hpp"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "GeometryCache.h"

#define LOCTEXT_NAMESPACE "FSkeletalMeshActionsListener"


/* Processor implementation */
namespace FSkeletalMeshProcessor_AutoSet_Internal
{
    using namespace MineFormatStringInternal;

    class FDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
    {
    private:
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
            // Without SourceControl
        }

        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {
            TArray<UObject *> ObjectToSave;
            uint16 const LodId = 0;

            for (auto SkIt = Assets.CreateConstIterator (); SkIt; ++SkIt) {
                // Do jobs
                auto const SkMesh = *SkIt;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *SkMesh->GetPathName ());

                // Generate abc dir path
                TArray<FString> AbcPathArray;
                MakeRelativeAbcDirPath (SkMesh->GetPathName(), AbcPathArray);

                // Found Materials
                TArray<FSkeletalMaterial> AllMats = SkMesh->GetMaterials ();

                // get sections number
                int SectionsNum = 0;
                if (SkMesh->GetResourceForRendering () && SkMesh->GetResourceForRendering ()->LODRenderData.Num () > 0) {
                    // Find Mat by Section
                    //TIndirectArray<FSkeletalMeshLODRenderData> &LodRenderData = SkMesh->GetResourceForRendering ()->LODRenderData;
                    //if (LodRenderData.IsValidIndex (LodId)) {
                    //    SectionsNum = LodRenderData[LodId].RenderSections.Num ();
                    //}
                    //for (int SectionId = 0; SectionId < SectionsNum; ++SectionId) {
                    //    uint16 const CurSectionMatId = LodRenderData[LodId].RenderSections[SectionId].MaterialIndex;
                    //    FName CurMatSlotName = AllMats[CurSectionMatId].MaterialSlotName;
                    //    FName CurMatImpName  = AllMats[CurSectionMatId].ImportedMaterialSlotName;
                    //    FString CurMatPath = AllMats[CurSectionMatId].MaterialInterface->GetPathName();
                    //    UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("MatImpName %s as SlotName %s @ Section %d; Path @ %s"),
                    //        *CurMatImpName.ToString(), *CurMatSlotName.ToString(), SectionId, *CurMatPath);
                    //}

                    // Find Mat by MatIndex
                    for (int MatId =0; MatId < AllMats.Num();++MatId)
                    {
                        FName CurMatSlotName = AllMats[MatId].MaterialSlotName;

                        for (auto AbcDirPath : AbcPathArray)
                        {
                            UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Searching @ %s"), *AbcDirPath);
                            FString MatchedPackagePath;
                            if (HasFoundClothAbcFile (CurMatSlotName, AbcDirPath, MatchedPackagePath))
                            {
                                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Found Matched Package @ %s"), *MatchedPackagePath);
                                // Load Asset to UObject to modify
                                
                                if (FAssetSourceControlHelper::IsSourceControlAvailable())
                                {
                                    FAssetSourceControlHelper::CheckOutFile (MatchedPackagePath);
                                }

                                auto const AbcAsset = LoadAsset(MatchedPackagePath);
                                if (IsValid(AbcAsset))
                                {
                                    UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Load as @ %s"), *AbcAsset->GetFullName ());
                                    auto const GeoCache = Cast<UGeometryCache> (AbcAsset);

                                    // check if abc type
                                    if (GeoCache == nullptr || GeoCache->StaticClass ()->GetFName () != UGeometryCache::StaticClass ()->GetFName ()) {
                                        UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s is not valid ABC GeometryCache"), *AbcAsset->GetFullName ());
                                        continue;
                                    }

                                    GeoCache->Modify ();
                                    TArray<UMaterialInterface*> GeoCacheMatArray = GeoCache->Materials;

                                    // Replace Current Mat
                                    for (uint16 GeoMatId=0;GeoMatId<GeoCacheMatArray.Num();++GeoMatId)
                                    {
                                        UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Change to Mat @ %s"), *AllMats[MatId].MaterialInterface->GetName ());
                                        GeoCacheMatArray[GeoMatId] = AllMats[MatId].MaterialInterface;
                                    }
                                    // Update Mat
                                    GeoCache->Materials = GeoCacheMatArray;
                                    ObjectToSave.Add (AbcAsset);
                                }
                            }
                        }
                    }
                }
            }
            UPackageTools::SavePackagesForObjects (ObjectToSave);
        }

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
            if (FPaths::DirectoryExists(AbcDirPath))
            {
                MatchedPackagePath = FPaths::ConvertRelativePathToFull(AbcDirPath / TEXT ("Cloth"), MatSlotName.ToString ());
                FPackageName::TryConvertFilenameToLongPackageName (MatchedPackagePath, MatchedPackagePath);
                UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Try to found Alembic @ %s"), *MatchedPackagePath);
                if (FPackageName::DoesPackageExist (MatchedPackagePath))
                {
                    return true;
                }
            }
            return false;
        }

        static void MakeRelativeAbcDirPath (const FString & MatPackagePath, TArray<FString> &AbcPathArray)
        {
            AbcPathArray.Empty ();
           
            FString TempDirPath;
            FPackageName::TryConvertLongPackageNameToFilename (MatPackagePath, TempDirPath);
            TempDirPath = FPaths::GetPath (TempDirPath) / TEXT ("Animations/Alembic");
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to found all Alembic Diectory @ %s"),*TempDirPath);
            if (FPaths::DirectoryExists(TempDirPath))
            {
                FDirectoryVisitor Visitor;
                FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*TempDirPath, Visitor);
                AbcPathArray = Visitor.DirectoryArray;
            }
        }

        static bool IsPackageFlagsSupportedForAssetLibrary (uint32 PackageFlags)
        {
            return (PackageFlags & (PKG_ContainsMap | PKG_PlayInEditor | PKG_ContainsMapData)) == 0;
        }

        static UObject* LoadAsset (const FString &AssetPath)
        {
            FString FailureReason;
            UObject *Result = LoadAsset (AssetPath, false, FailureReason);
            if (Result == nullptr) {
                UE_LOG (LogMineCustomToolEditor, Error, TEXT ("LoadAsset. Failed to load asset: %s"), *FailureReason);
            }
            return Result;
        }

        static UObject *LoadAsset (const FString &AssetPath, bool bAllowMapAsset, FString &OutFailureReason)
        {
            FAssetData AssetData = FindAssetDataFromAnyPath (AssetPath, OutFailureReason);
            if (!AssetData.IsValid ()) {
                return nullptr;
            }
            return LoadAsset (AssetData, bAllowMapAsset, OutFailureReason);
        }

        static UObject *LoadAsset (const FAssetData &AssetData, bool bAllowMapAsset, FString &OutFailureReason)
        {
            if (!AssetData.IsValid ()) {
                return nullptr;
            }

            if (!bAllowMapAsset) {
                if (FEditorFileUtils::IsMapPackageAsset (AssetData.ObjectPath.ToString ())
                    || !IsPackageFlagsSupportedForAssetLibrary (AssetData.PackageFlags)) {
                    OutFailureReason = FString::Printf (TEXT ("The AssetData '%s' is not accessible because it is of type Map/Level."), *AssetData.ObjectPath.ToString ());
                    return nullptr;
                }
            }

            UObject *FoundObject = AssetData.GetAsset ();
            if (!FoundObject || FoundObject->IsPendingKill ()) {
                OutFailureReason = FString::Printf (TEXT ("The asset '%s' exists but was not able to be loaded."), *AssetData.ObjectPath.ToString ());
            }
            else if (!FoundObject->IsAsset ()) {
                OutFailureReason = FString::Printf (TEXT ("'%s' is not a valid asset."), *AssetData.ObjectPath.ToString ());
                FoundObject = nullptr;
            }
            return FoundObject;
        }

        /** Remove Class from "Class /Game/MyFolder/MyAsset" */
        static FString RemoveFullName (const FString &AnyAssetPath, FString &OutFailureReason)
        {
            FString Result = AnyAssetPath.TrimStartAndEnd ();
            int32 NumberOfSpace = Algo::Count (AnyAssetPath, TEXT (' '));

            if (NumberOfSpace == 0) {
                return MoveTemp (Result);
            }
            else if (NumberOfSpace > 1) {
                OutFailureReason = FString::Printf (TEXT ("Can't convert path '%s' because there are too many spaces."), *AnyAssetPath);
                return FString ();
            }
            else// if (NumberOfSpace == 1)
            {
                int32 FoundIndex = 0;
                AnyAssetPath.FindChar (TEXT (' '), FoundIndex);
                check (FoundIndex > INDEX_NONE && FoundIndex < AnyAssetPath.Len ()); // because of TrimStartAndEnd

                // Confirm that it's a valid Class
                FString ClassName = AnyAssetPath.Left (FoundIndex);

                // Convert \ to /
                ClassName.ReplaceInline (TEXT ("\\"), TEXT ("/"), ESearchCase::CaseSensitive);

                // Test ClassName for invalid Char
                const int32 StrLen = FCString::Strlen (INVALID_OBJECTNAME_CHARACTERS);
                for (int32 Index = 0; Index < StrLen; ++Index) {
                    int32 InvalidFoundIndex = 0;
                    if (ClassName.FindChar (INVALID_OBJECTNAME_CHARACTERS[Index], InvalidFoundIndex)) {
                        OutFailureReason = FString::Printf (TEXT ("Can't convert the path %s because it contains invalid characters (probably spaces)."), *AnyAssetPath);
                        return FString ();
                    }
                }

                // Return the path without the Class name
                return AnyAssetPath.Mid (FoundIndex + 1);
            }
        }

        // Test for invalid characters
        static bool IsAValidPath (const FString &Path, const TCHAR *InvalidChar, FString &OutFailureReason)
        {
            // Like !FName::IsValidGroupName(Path)), but with another list and no conversion to from FName
            // InvalidChar may be INVALID_OBJECTPATH_CHARACTERS or INVALID_LONGPACKAGE_CHARACTERS or ...
            const int32 StrLen = FCString::Strlen (InvalidChar);
            for (int32 Index = 0; Index < StrLen; ++Index) {
                int32 FoundIndex = 0;
                if (Path.FindChar (InvalidChar[Index], FoundIndex)) {
                    OutFailureReason = FString::Printf (TEXT ("Can't convert the path %s because it contains invalid characters."), *Path);
                    return false;
                }
            }

            if (Path.Len () > FPlatformMisc::GetMaxPathLength ()) {
                OutFailureReason = FString::Printf (TEXT ("Can't convert the path because it is too long (%d characters). This may interfere with cooking for consoles. Unreal filenames should be no longer than %d characters. Full path value: %s"), Path.Len (), FPlatformMisc::GetMaxPathLength (), *Path);
                return false;
            }
            return true;
        }

        static bool HasValidRoot (const FString &ObjectPath)
        {
            FString Filename;
            bool bValidRoot = true;
            if (!ObjectPath.IsEmpty () && ObjectPath[ObjectPath.Len () - 1] == TEXT ('/')) {
                bValidRoot = FPackageName::TryConvertLongPackageNameToFilename (ObjectPath, Filename);
            }
            else {
                FString ObjectPathWithSlash = ObjectPath;
                ObjectPathWithSlash.AppendChar (TEXT ('/'));
                bValidRoot = FPackageName::TryConvertLongPackageNameToFilename (ObjectPathWithSlash, Filename);
            }

            return bValidRoot;
        }

        static FString ConvertAnyPathToObjectPath (const FString &AnyAssetPath, FString &OutFailureReason)
        {
            if (AnyAssetPath.Len () < 2) // minimal length to have /G
            {
                OutFailureReason = FString::Printf (TEXT ("Can't convert the path '%s' because the Root path need to be specified. ie /Game/"), *AnyAssetPath);
                return FString ();
            }

            // Remove class name from Reference Path
            FString TextPath = FPackageName::ExportTextPathToObjectPath (AnyAssetPath);

            // Remove class name Fullname
            TextPath = RemoveFullName (TextPath, OutFailureReason);
            if (TextPath.IsEmpty ()) {
                return FString ();
            }

            // Extract the subobject path if any
            FString SubObjectPath;
            int32 SubObjectDelimiterIdx;
            if (TextPath.FindChar (SUBOBJECT_DELIMITER_CHAR, SubObjectDelimiterIdx)) {
                SubObjectPath = TextPath.Mid (SubObjectDelimiterIdx + 1);
                TextPath.LeftInline (SubObjectDelimiterIdx);
            }

            // Convert \ to /
            TextPath.ReplaceInline (TEXT ("\\"), TEXT ("/"), ESearchCase::CaseSensitive);
            FPaths::RemoveDuplicateSlashes (TextPath);

            // Get asset full name, i.e."PackageName.ObjectName:InnerAssetName.2ndInnerAssetName" from "/Game/Folder/PackageName.ObjectName:InnerAssetName.2ndInnerAssetName"
            FString AssetFullName;
            {
                // Get everything after the last slash
                int32 IndexOfLastSlash = INDEX_NONE;
                TextPath.FindLastChar ('/', IndexOfLastSlash);

                FString Folders = TextPath.Left (IndexOfLastSlash);
                // Test for invalid characters
                if (!IsAValidPath (Folders, INVALID_LONGPACKAGE_CHARACTERS, OutFailureReason)) {
                    return FString ();
                }

                AssetFullName = TextPath.Mid (IndexOfLastSlash + 1);
            }

            // Get the object name
            FString ObjectName = FPackageName::ObjectPathToObjectName (AssetFullName);
            if (ObjectName.IsEmpty ()) {
                OutFailureReason = FString::Printf (TEXT ("Can't convert the path '%s' because it doesn't contain an asset name."), *AnyAssetPath);
                return FString ();
            }

            // Test for invalid characters
            if (!IsAValidPath (ObjectName, INVALID_OBJECTNAME_CHARACTERS, OutFailureReason)) {
                return FString ();
            }

            // Confirm that we have a valid Root Package and get the valid PackagePath /Game/MyFolder/MyAsset
            FString PackagePath;
            if (!FPackageName::TryConvertFilenameToLongPackageName (TextPath, PackagePath, &OutFailureReason)) {
                return FString ();
            }

            if (PackagePath.Len () == 0) {
                OutFailureReason = FString::Printf (TEXT ("Can't convert path '%s' because the PackagePath is empty."), *AnyAssetPath);
                return FString ();
            }

            if (PackagePath[0] != TEXT ('/')) {
                OutFailureReason = FString::Printf (TEXT ("Can't convert path '%s' because the PackagePath '%s' doesn't start with a '/'."), *AnyAssetPath, *PackagePath);
                return FString ();
            }

            FString ObjectPath = FString::Printf (TEXT ("%s.%s"), *PackagePath, *ObjectName);

            if (FPackageName::IsScriptPackage (ObjectPath)) {
                OutFailureReason = FString::Printf (TEXT ("Can't convert the path '%s' because it start with /Script/"), *AnyAssetPath);
                return FString ();
            }
            if (FPackageName::IsMemoryPackage (ObjectPath)) {
                OutFailureReason = FString::Printf (TEXT ("Can't convert the path '%s' because it start with /Memory/"), *AnyAssetPath);
                return FString ();
            }

            // Confirm that the PackagePath starts with a valid root
            if (!HasValidRoot (PackagePath)) {
                OutFailureReason = FString::Printf (TEXT ("Can't convert the path '%s' because it does not map to a root."), *AnyAssetPath);
                return FString ();
            }

            return ObjectPath;
        }

        static FAssetData FindAssetDataFromAnyPath (const FString &AnyAssetPath, FString &OutFailureReason)
        {
            FString ObjectPath = ConvertAnyPathToObjectPath (AnyAssetPath, OutFailureReason);
            if (ObjectPath.IsEmpty ()) {
                return FAssetData ();
            }

            if (FEditorFileUtils::IsMapPackageAsset (ObjectPath)) {
                OutFailureReason = FString::Printf (TEXT ("The AssetData '%s' is not accessible because it is of type Map/Level."), *ObjectPath);
                return FAssetData ();
            }

            FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule> ("AssetRegistry");
            FAssetData AssetData = AssetRegistryModule.Get ().GetAssetByObjectPath (*ObjectPath);
            if (!AssetData.IsValid ()) {
                OutFailureReason = FString::Printf (TEXT ("The AssetData '%s' could not be found in the Content Browser."), *ObjectPath);
                return FAssetData ();
            }

            // Prevent loading a umap...
            if (!IsPackageFlagsSupportedForAssetLibrary (AssetData.PackageFlags)) {
                OutFailureReason = FString::Printf (TEXT ("The AssetData '%s' is not accessible because it is of type Map/Level."), *ObjectPath);
                return FAssetData ();
            }
            return AssetData;
        }
    };
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
                "Auto set SkeletalMesh",
                "Auto set configs for selected SkeletalMesh assets.",
                FSkeletalMeshProcessor_AutoSet
            );

            // 1
            FORMAT_COMMAND_INFO (1,
                "Set ABC Mat",
                "Auto set Alembic Static Mesh Materials for selected SkeletalMesh assets.",
                FSkeletalMeshProcessor_AbcClothBindToMatSlots

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