﻿#pragma once
#include "CustomDataType/ExampleDataFactory.h"
#include "CustomDataType/ExampleData.h"
#include "EditorReimportHandler.h" // FReimportHandler
#include "ReimportExampleDataFactory.generated.h"

UCLASS()
class UReimportExampleDataFactory : public UExampleDataFactory, public FReimportHandler
{
    GENERATED_BODY ()

    // Begin FReimportHandler interface
    virtual bool CanReimport (UObject *Obj, TArray<FString> &OutFilenames) override;
    virtual void SetReimportPaths (UObject *Obj, const TArray<FString> &NewReimportPaths) override;
    virtual EReimportResult::Type Reimport (UObject *Obj) override;
    // End FReimportHandler interface
};