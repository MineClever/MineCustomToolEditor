// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "MineCustomToolEditor/Public/DetailsCustomization/ExampleActor.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeExampleActor() {}
// Cross Module References
	MINECUSTOMTOOLEDITOR_API UClass* Z_Construct_UClass_AExampleActor_NoRegister();
	MINECUSTOMTOOLEDITOR_API UClass* Z_Construct_UClass_AExampleActor();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	UPackage* Z_Construct_UPackage__Script_MineCustomToolEditor();
// End Cross Module References
	void AExampleActor::StaticRegisterNativesAExampleActor()
	{
	}
	UClass* Z_Construct_UClass_AExampleActor_NoRegister()
	{
		return AExampleActor::StaticClass();
	}
	struct Z_Construct_UClass_AExampleActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_bOption1_MetaData[];
#endif
		static void NewProp_bOption1_SetBit(void* Obj);
		static const UE4CodeGen_Private::FBoolPropertyParams NewProp_bOption1;
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_bOption2_MetaData[];
#endif
		static void NewProp_bOption2_SetBit(void* Obj);
		static const UE4CodeGen_Private::FBoolPropertyParams NewProp_bOption2;
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_testInt_MetaData[];
#endif
		static const UE4CodeGen_Private::FUnsizedIntPropertyParams NewProp_testInt;
		static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AExampleActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_MineCustomToolEditor,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AExampleActor_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/*\n * In this class,\n * we add 2 booleans in \"Options\" category,\n * and an integer in \"Test\" category.\n *\n * Remember to add \"MODULE_API\" in front of class name to export it from game module,\n * otherwise we cannot use it in editor module.\n */" },
		{ "IncludePath", "DetailsCustomization/ExampleActor.h" },
		{ "ModuleRelativePath", "Public/DetailsCustomization/ExampleActor.h" },
		{ "ToolTip", "* In this class,\n* we add 2 booleans in \"Options\" category,\n* and an integer in \"Test\" category.\n*\n* Remember to add \"MODULE_API\" in front of class name to export it from game module,\n* otherwise we cannot use it in editor module." },
	};
#endif
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption1_MetaData[] = {
		{ "Category", "Options" },
		{ "ModuleRelativePath", "Public/DetailsCustomization/ExampleActor.h" },
	};
#endif
	void Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption1_SetBit(void* Obj)
	{
		((AExampleActor*)Obj)->bOption1 = 1;
	}
	const UE4CodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption1 = { "bOption1", nullptr, (EPropertyFlags)0x0010000000000001, UE4CodeGen_Private::EPropertyGenFlags::Bool | UE4CodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, sizeof(bool), sizeof(AExampleActor), &Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption1_SetBit, METADATA_PARAMS(Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption1_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption1_MetaData)) };
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption2_MetaData[] = {
		{ "Category", "Options" },
		{ "ModuleRelativePath", "Public/DetailsCustomization/ExampleActor.h" },
	};
#endif
	void Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption2_SetBit(void* Obj)
	{
		((AExampleActor*)Obj)->bOption2 = 1;
	}
	const UE4CodeGen_Private::FBoolPropertyParams Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption2 = { "bOption2", nullptr, (EPropertyFlags)0x0010000000000001, UE4CodeGen_Private::EPropertyGenFlags::Bool | UE4CodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, 1, sizeof(bool), sizeof(AExampleActor), &Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption2_SetBit, METADATA_PARAMS(Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption2_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption2_MetaData)) };
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AExampleActor_Statics::NewProp_testInt_MetaData[] = {
		{ "Category", "Test" },
		{ "ModuleRelativePath", "Public/DetailsCustomization/ExampleActor.h" },
	};
#endif
	const UE4CodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UClass_AExampleActor_Statics::NewProp_testInt = { "testInt", nullptr, (EPropertyFlags)0x0010000000000001, UE4CodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(AExampleActor, testInt), METADATA_PARAMS(Z_Construct_UClass_AExampleActor_Statics::NewProp_testInt_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AExampleActor_Statics::NewProp_testInt_MetaData)) };
	const UE4CodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AExampleActor_Statics::PropPointers[] = {
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption1,
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AExampleActor_Statics::NewProp_bOption2,
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AExampleActor_Statics::NewProp_testInt,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AExampleActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AExampleActor>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_AExampleActor_Statics::ClassParams = {
		&AExampleActor::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AExampleActor_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AExampleActor_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AExampleActor_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AExampleActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AExampleActor()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_AExampleActor_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(AExampleActor, 3110485761);
	template<> MINECUSTOMTOOLEDITOR_API UClass* StaticClass<AExampleActor>()
	{
		return AExampleActor::StaticClass();
	}
	static FCompiledInDefer Z_CompiledInDefer_UClass_AExampleActor(Z_Construct_UClass_AExampleActor, &AExampleActor::StaticClass, TEXT("/Script/MineCustomToolEditor"), TEXT("AExampleActor"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(AExampleActor);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
