// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "MineCustomToolEditor/Public/CustomDataType/ExampleData.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeExampleData() {}
// Cross Module References
	MINECUSTOMTOOLEDITOR_API UClass* Z_Construct_UClass_UExampleData_NoRegister();
	MINECUSTOMTOOLEDITOR_API UClass* Z_Construct_UClass_UExampleData();
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
	UPackage* Z_Construct_UPackage__Script_MineCustomToolEditor();
// End Cross Module References
	void UExampleData::StaticRegisterNativesUExampleData()
	{
	}
	UClass* Z_Construct_UClass_UExampleData_NoRegister()
	{
		return UExampleData::StaticClass();
	}
	struct Z_Construct_UClass_UExampleData_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_ExampleString_MetaData[];
#endif
		static const UE4CodeGen_Private::FStrPropertyParams NewProp_ExampleString;
#if WITH_EDITORONLY_DATA
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_SourceFilePath_MetaData[];
#endif
		static const UE4CodeGen_Private::FStrPropertyParams NewProp_SourceFilePath;
#endif // WITH_EDITORONLY_DATA
		static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_EDITORONLY_DATA
#endif // WITH_EDITORONLY_DATA
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UExampleData_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UObject,
		(UObject* (*)())Z_Construct_UPackage__Script_MineCustomToolEditor,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UExampleData_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Comment", "/*\n * For simple data,\n * you can just inherit from UDataAsset class,\n * then you can create your data object in Unreal content browser:\n * Add New ?? miscellaneous ?? Data Asset\n */" },
		{ "IncludePath", "CustomDataType/ExampleData.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/CustomDataType/ExampleData.h" },
		{ "ToolTip", "* For simple data,\n* you can just inherit from UDataAsset class,\n* then you can create your data object in Unreal content browser:\n* Add New ?? miscellaneous ?? Data Asset" },
	};
#endif
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UExampleData_Statics::NewProp_ExampleString_MetaData[] = {
		{ "Category", "Properties" },
		{ "ModuleRelativePath", "Public/CustomDataType/ExampleData.h" },
	};
#endif
	const UE4CodeGen_Private::FStrPropertyParams Z_Construct_UClass_UExampleData_Statics::NewProp_ExampleString = { "ExampleString", nullptr, (EPropertyFlags)0x0010000000000001, UE4CodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(UExampleData, ExampleString), METADATA_PARAMS(Z_Construct_UClass_UExampleData_Statics::NewProp_ExampleString_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_UExampleData_Statics::NewProp_ExampleString_MetaData)) };
#if WITH_EDITORONLY_DATA
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UExampleData_Statics::NewProp_SourceFilePath_MetaData[] = {
		{ "Category", "SourceAsset" },
		{ "ModuleRelativePath", "Public/CustomDataType/ExampleData.h" },
	};
#endif
	const UE4CodeGen_Private::FStrPropertyParams Z_Construct_UClass_UExampleData_Statics::NewProp_SourceFilePath = { "SourceFilePath", nullptr, (EPropertyFlags)0x0010000800020001, UE4CodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(UExampleData, SourceFilePath), METADATA_PARAMS(Z_Construct_UClass_UExampleData_Statics::NewProp_SourceFilePath_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_UExampleData_Statics::NewProp_SourceFilePath_MetaData)) };
#endif // WITH_EDITORONLY_DATA
	const UE4CodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UExampleData_Statics::PropPointers[] = {
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UExampleData_Statics::NewProp_ExampleString,
#if WITH_EDITORONLY_DATA
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UExampleData_Statics::NewProp_SourceFilePath,
#endif // WITH_EDITORONLY_DATA
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UExampleData_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UExampleData>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_UExampleData_Statics::ClassParams = {
		&UExampleData::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UExampleData_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_UExampleData_Statics::PropPointers),
		0,
		0x001000A0u,
		METADATA_PARAMS(Z_Construct_UClass_UExampleData_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UExampleData_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UExampleData()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_UExampleData_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(UExampleData, 1429279403);
	template<> MINECUSTOMTOOLEDITOR_API UClass* StaticClass<UExampleData>()
	{
		return UExampleData::StaticClass();
	}
	static FCompiledInDefer Z_CompiledInDefer_UClass_UExampleData(Z_Construct_UClass_UExampleData, &UExampleData::StaticClass, TEXT("/Script/MineCustomToolEditor"), TEXT("UExampleData"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(UExampleData);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
