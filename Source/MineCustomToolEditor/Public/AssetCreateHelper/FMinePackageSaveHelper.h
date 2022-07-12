#pragma once
#include "CoreMinimal.h"
#include "UObject/Package.h"

namespace MinePackageHelperInternal
{
    FORCEINLINE void SaveUObjectPackage (UObject *InObject)
    {
        InObject->GetPackage ()->MarkPackageDirty ();
        UPackage::Save (InObject->GetPackage (),
            InObject,
            EObjectFlags::RF_Public | ::RF_Standalone,
            *InObject->GetPackage ()->GetName (),
            GError,
            nullptr,
            true,
            true,
            SAVE_NoError | SAVE_Async
        );
    }
}
