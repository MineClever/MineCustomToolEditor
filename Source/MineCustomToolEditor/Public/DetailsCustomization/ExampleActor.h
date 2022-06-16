#pragma once
#include "MineMouduleDefine.h"
#include "ExampleActor.generated.h"

/*
 * In this class,
 * we add 2 booleans in "Options" category,
 * and an integer in "Test" category.
 *
 * Remember to add "MODULE_API" in front of class name to export it from game module,
 * otherwise we cannot use it in editor module.
 */
UCLASS()
class CURRENT_CUSTOM_MODULE_API AExampleActor : public AActor
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, Category = "Options")
        bool bOption1 = false;

    UPROPERTY (EditAnywhere, Category = "Options")
        bool bOption2 = false;

    UPROPERTY (EditAnywhere, Category = "Test")
        int testInt = 0;
};