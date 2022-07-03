﻿#pragma once
#include "CoreMinimal.h"
#include "MineMouduleDefine.h"
// Source Control
#include "ISourceControlModule.h"
#include "ISourceControlRevision.h"
#include "ISourceControlProvider.h"
#include "SourceControlHelpers.h"


class FAssetSourceControlHelper
{
public:
    static ISourceControlModule& SourceControlModule ;

public:

    static FString GetCurrentProviderFName ()
    {
        if (IsSourceControlAvailable())
        {
            return SourceControlModule.GetProvider().GetName ().ToString();
        } else
        {
            return FString(TEXT("None"));
        }
    }

    static bool IsSourceControlAvailable ()
    {
        return SourceControlModule.IsEnabled () && SourceControlModule.GetProvider ().IsAvailable ();
    }

    /**
     * @brief Use currently set source control provider to check out specified files.Wrap for silent mode
     * @param Files Files to check out
     * @return true if succeeded
     */
    static bool CheckOutFiles (const TArray<FString> & Files)
    {
        return USourceControlHelpers::CheckOutFiles (Files, true);
    }

    static bool CheckOutFiles (const FString &File)
    {
        return USourceControlHelpers::CheckOutFile (File, true);
    }

};

ISourceControlModule& FAssetSourceControlHelper::SourceControlModule = ISourceControlModule::Get ();