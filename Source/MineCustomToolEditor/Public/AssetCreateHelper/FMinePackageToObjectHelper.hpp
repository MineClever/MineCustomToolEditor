#pragma once
#include "MineMouduleDefine.h"
#include "CoreMinimal.h"
#include "FileHelpers.h"
#include "Algo/Count.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ConfigIO/ConfigIO.h"

namespace MinePackageLoadHelper
{
    FORCEINLINE bool IsPackageFlagsSupportedForAssetLibrary (uint32 PackageFlags)
    {
        return (PackageFlags & (PKG_ContainsMap | PKG_PlayInEditor | PKG_ContainsMapData)) == 0;
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
        //FAssetData AssetData = AssetRegistryModule.Get ().GetAssetByObjectPath (*ObjectPath);

        FAssetData AssetData;
        UE::AssetRegistry::EExists IsValidAsset = AssetRegistryModule.Get().TryGetAssetByObjectPath(
            FSoftObjectPath(ObjectPath), AssetData);
        if (IsValidAsset != UE::AssetRegistry::EExists::Exists) {
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

    static UObject *LoadAsset (const FAssetData &AssetData, bool bAllowMapAsset, FString &OutFailureReason)
    {
        if (!AssetData.IsValid ()) {
            return nullptr;
        }

        if (!bAllowMapAsset) {
            if (FEditorFileUtils::IsMapPackageAsset (AssetData.GetObjectPathString ())
                || !IsPackageFlagsSupportedForAssetLibrary (AssetData.PackageFlags)) {
                OutFailureReason = FString::Printf (TEXT ("The AssetData '%s' is not accessible because it is of type Map/Level."), *AssetData.GetObjectPathString());
                return nullptr;
            }
        }

        UObject *FoundObject = AssetData.GetAsset ();
        if (!FoundObject || FoundObject->IsPendingKill ()) {
            OutFailureReason = FString::Printf (TEXT ("The asset '%s' exists but was not able to be loaded."), *AssetData.GetObjectPathString());
        }
        else if (!FoundObject->IsAsset ()) {
            OutFailureReason = FString::Printf (TEXT ("'%s' is not a valid asset."), *AssetData.GetObjectPathString());
            FoundObject = nullptr;
        }
        return FoundObject;
    }

    static UObject *LoadAsset (const FString &AssetPath, bool bAllowMapAsset, FString &OutFailureReason)
    {
        FAssetData const AssetData = FindAssetDataFromAnyPath (AssetPath, OutFailureReason);
        if (!AssetData.IsValid ()) {
            return nullptr;
        }
        return LoadAsset (AssetData, bAllowMapAsset, OutFailureReason);
    }

    static UObject *LoadAsset (const FString &AssetPath)
    {
        FString FailureReason;
        UObject *Result = LoadAsset (AssetPath, false, FailureReason);
        if (Result == nullptr) {
            UE_LOG (LogMineCustomToolEditor, Error, TEXT ("LoadAsset. Failed to load asset: %s"), *FailureReason);
        }
        return Result;
    }

}

namespace MineMaterialPackageHelper
{

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

        for (auto MaterialRuleDir : MaterialsRuleDirArray) {
            FString TempDirPath;
            FPackageName::TryConvertLongPackageNameToFilename (MatPackagePath, TempDirPath);
            TempDirPath = FPaths::GetPath (TempDirPath) / MaterialRuleDir;
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("TempPath as @ %s"), *TempDirPath);
            if (FPaths::DirectoryExists (TempDirPath)) {
                ValidPathArray.AddUnique (TempDirPath);
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

}
