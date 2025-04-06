#pragma once
#include "CoreMinimal.h"
#include "PackageHelperFunctions.h"
#include "UObject/Package.h"

namespace MinePackageHelperInternal
{
<<<<<<< HEAD
    FORCEINLINE void SaveUObjectPackage (const UObject* InObject)
    {
        
        SavePackageHelper(
            InObject->GetPackage(),
            InObject->GetPackage()->GetOutermost()->GetPathName(),
            EObjectFlags::RF_Public | ::RF_Standalone, GError);
=======
    FORCEINLINE void SaveUObjectPackage (UObject* InObject)
    {
        auto &&ObjectPackage = InObject->GetPackage();
        ObjectPackage->MarkPackageDirty ();
        UPackage::Save (ObjectPackage,
            InObject,
            EObjectFlags::RF_Public | ::RF_Standalone,
            *(ObjectPackage->GetName ()),
            GError,
            nullptr,
            true,
            true,
            SAVE_NoError | SAVE_Async
        );
>>>>>>> c18686737fe05066a1172762c9375b9d4f20bcef
    }
}
