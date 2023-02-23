#pragma once
#include "CoreMinimal.h"
#include "UObject/Package.h"

namespace MinePackageHelperInternal
{
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
    }
}
