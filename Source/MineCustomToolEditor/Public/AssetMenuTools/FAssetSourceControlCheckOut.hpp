#pragma once
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
        const bool bCheckOut = IsSourceControlAvailable ();
        if (bCheckOut) {
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to CheckOut %d files"), Files.Num ());
            return USourceControlHelpers::CheckOutFiles (Files, true);
        }
        else return bCheckOut;

    }

    static bool CheckOutFiles (const FString &File)
    {
        const bool bCheckOut = IsSourceControlAvailable ();
        if (bCheckOut) {
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to CheckOut file : %s"), *File);
            return USourceControlHelpers::CheckOutFile (File, true);
        }
        else return bCheckOut;
    }

};

ISourceControlModule& FAssetSourceControlHelper::SourceControlModule = ISourceControlModule::Get ();