#pragma once
#include "CoreMinimal.h"
#include "PackageHelperFunctions.h"
#include "UObject/Package.h"

namespace MinePackageHelperInternal
{

    FORCEINLINE void SaveUObjectPackage (const UObject* InObject)
    {
        
        SavePackageHelper(
            InObject->GetPackage(),
            InObject->GetPackage()->GetOutermost()->GetPathName(),
            EObjectFlags::RF_Public | ::RF_Standalone, GError);
    }
}
