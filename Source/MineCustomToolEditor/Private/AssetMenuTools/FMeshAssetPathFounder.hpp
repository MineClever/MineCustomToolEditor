// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConfigIO/ConfigIO.h"

/**
 * 
 */

namespace FMeshAssetPathFounder_Internal
{
    class FDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
    {
    protected:
        virtual bool Visit (const TCHAR *FilenameOrDirectory, bool bIsDirectory) override
        {
            if (bIsDirectory) {
                FString TempPath;
                // FPackageName::TryConvertFilenameToLongPackageName (FString (FilenameOrDirectory), TempPath);
                DirectoryArray.AddUnique (FString (FilenameOrDirectory));
            }
            return true;
        }
    public:
        TArray<FString> DirectoryArray;
    };

    /**
     * @brief :Check if Slot name based Abc File in inputted Abc Directory Path
     * @param MatSlotName :FName
     * @param AbcDirPath :FString
     * @param MatchedPackagePath :FString
     * @param MatchedObjectPath :FString
     * @return : if Matched Abc StaticMesh Name
     */
    static bool HasFoundClothAbcFile (const FName &MatSlotName, const FString &AbcDirPath, FString &MatchedPackagePath)
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

    static void MakeRelativeAbcDirPath (const FString &MatPackagePath, TArray<FString> &AbcPathArray)
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
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to found all Alembic Diectory @ [%s]"), *TempDirPath);
        if (FPaths::DirectoryExists (TempDirPath)) {
            FDirectoryVisitor Visitor;
            FPlatformFileManager::Get ().GetPlatformFile ().IterateDirectory (*TempDirPath, Visitor);
            AbcPathArray = Visitor.DirectoryArray;
        }
        else
        {
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("The Alembic Diectory @ [%s] is not valid"), *TempDirPath);
        }
    }

}
