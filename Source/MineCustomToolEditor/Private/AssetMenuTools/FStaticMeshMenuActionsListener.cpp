#include "AssetMenuTools/FStaticMeshMenuActionsListener.h"
#include "AssetMenuTools/TAssetsProcessorFormSelection.hpp"
#include "AssetCreateHelper/FMinePackageSaveHelper.h"
#include "AssetCreateHelper/FMinePackageToObjectHelper.hpp"
#include "ConfigIO/ConfigIO.h"
#include "Internationalization/Regex.h"
#include "AssetMenuTools/FSkeletalMeshActionListener.h"
#include "GeometryCache.h"
#include "GeometryCacheTrack.h"
#include "./FMeshAssetPathFounder.hpp"
#include "UObject/ObjectSaveContext.h"


//////////////////////////////////////////////////////////////////////////
// Start LocText NameSpace
#define LOCTEXT_NAMESPACE "FStaticMeshMenuActionsListener"



//////////////////////////////////////////////////////////////////////////
// TAssetsProcessorFormSelection_UStaticMesh

class FAssetsProcessorFormSelection_UStaticMesh_PrintName : public TAssetsProcessorFormSelection_Builder<UStaticMesh>
{
public:
	FAssetsProcessorFormSelection_UStaticMesh_PrintName () :TAssetsProcessorFormSelection_Builder<UStaticMesh>(false)
	{
	    
	}

	virtual void ProcessAssets (TArray<UStaticMesh *> &Assets) override
	{
		FString StringArrayToCopy = "";
		UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Static Mesh Copy Assets Path Processor Run !! "));
		uint32 LoopCount = 1;
		for (auto const Asset : Assets)
		{
			const FString TempAssetPathName = Asset->GetPackage ()->GetPathName ();
			UE_LOG (LogMineCustomToolEditor, Log, TEXT ("%d : %s"), LoopCount, *(TempAssetPathName));
			GEngine->AddOnScreenDebugMessage (-1, 5.f, FColor::Blue, *(TempAssetPathName));
			StringArrayToCopy.Append (TempAssetPathName);
			StringArrayToCopy.Append ("\n");
			++LoopCount;
		}
		
		FPlatformMisc::ClipboardCopy (*StringArrayToCopy);
	};
};


class FAssetsProcessorFormSelection_UStaticMesh_SetMeshHighPrecision : public TAssetsProcessorFormSelection_Builder<UStaticMesh>
{
public:

	virtual void ProcessAssets (TArray<UStaticMesh *> &Assets) override
	{
		UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Static Mesh Set High Precision UV|Tangent Processor Run !! "));

		// Unreal AssetSubSystem
		UAssetEditorSubsystem *const AssetSubSystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();

		// TODO: Read from Config
		float const NewDFResolutionScale = 24;

		for (auto const Asset : Assets) {
			uint32 const SourceModelNums = Asset->GetNumSourceModels ();
			if (SourceModelNums > 0) {

				// Get Asset WeakPtr cast to UObject (Safely)
				TWeakObjectPtr<UStaticMesh> WeakAssetPtr = Asset;
				UObject *AssetObject = Cast<UObject> (WeakAssetPtr);
				AssetSubSystem->CloseAllEditorsForAsset (AssetObject);

				Asset->Modify ();

				for (uint32 i = 0; i < SourceModelNums; ++i) {
					auto &&Model = Asset->GetSourceModel (i);
					Model.BuildSettings.bUseFullPrecisionUVs = 1;
					Model.BuildSettings.bUseHighPrecisionTangentBasis = 1;
				}
				Asset->Build ();
				Asset->PostSaveRoot (true);

				// Save!!
				MinePackageHelperInternal::SaveUObjectPackage (Asset);
			}
		}
	};
};


class FAssetsProcessorFormSelection_UStaticMesh_SetMeshDF : public TAssetsProcessorFormSelection_Builder<UStaticMesh>
{
public:

	virtual void ProcessAssets (TArray<UStaticMesh *> &Assets) override
	{
		UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Static Mesh Build Distance Filed Processor Run !! "));

		// Unreal AssetSubSystem
	    UAssetEditorSubsystem * const AssetSubSystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();
		auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
		bool const bCustom = ConfigSettings->bUseCustomStaticMeshProcessorConfig;
        float const NewDFResolutionScale = bCustom? ConfigSettings->ConfigDistanceFiledResolutionScale : 24;

        for (auto const Asset : Assets)
		{
			uint32 const SourceModelNums = Asset->GetNumSourceModels ();
			if (SourceModelNums > 0) {

				// Get Asset WeakPtr cast to UObject (Safely)
				TWeakObjectPtr<UStaticMesh> WeakAssetPtr = Asset;
				UObject *AssetObject = Cast<UObject> (WeakAssetPtr);
				AssetSubSystem->CloseAllEditorsForAsset (AssetObject);

				Asset->Modify ();
				
				for (uint32 i = 0; i < SourceModelNums; ++i) {
                    auto &&Model = Asset->GetSourceModel (i);
					//FObjectEditorUtils::SetPropertyValue<>;
					Model.BuildSettings.bGenerateDistanceFieldAsIfTwoSided = 1;
					Model.BuildSettings.DistanceFieldResolutionScale = NewDFResolutionScale;
					Asset->DistanceFieldSelfShadowBias = 0.02;
					Asset->bGenerateMeshDistanceField = 1;
				}
				Asset->PostSaveRoot (true);
				Asset->Build ();

				// Save!!
				MinePackageHelperInternal::SaveUObjectPackage (Asset);
			}
		}
	};
};


class FAssetsProcessorFormSelection_UStaticMesh_RemoveDF : public TAssetsProcessorFormSelection_Builder<UStaticMesh>
{
public:

	virtual void ProcessAssets (TArray<UStaticMesh *> &Assets) override
	{
		UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Static Mesh Remove Distance Filed Processor Run !! "));

		// Unreal AssetSubSystem
		UAssetEditorSubsystem *const AssetSubSystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();

		float const NewDFResolutionScale = 24;

		for (auto const Asset : Assets) {
			uint32 const SourceModelNums = Asset->GetNumSourceModels ();
			if (SourceModelNums > 0) {

				// Get Asset WeakPtr cast to UObject (Safely)
				TWeakObjectPtr<UStaticMesh> WeakAssetPtr = Asset;
				UObject *AssetObject = Cast<UObject> (WeakAssetPtr);
				AssetSubSystem->CloseAllEditorsForAsset (AssetObject);

				Asset->Modify ();
				Asset->bGenerateMeshDistanceField = 0;
				Asset->PostSaveRoot (true);
				Asset->Build ();

				// Save!!
				MinePackageHelperInternal::SaveUObjectPackage (Asset);

			}
		}
	};
};


class FAssetsProcessorFormSelection_UStaticMesh_AutoBindMat : public TAssetsProcessorFormSelection_Builder<UStaticMesh>
{
public:

	virtual void ProcessAssets (TArray<UStaticMesh *> &Assets) override
	{
		TArray<UObject *> ObjectToSave;

		for (auto const StaticMesh : Assets)
		{
			UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *StaticMesh->GetPathName ());
			// Generate abc dir path
			TArray<FString> MatDirPathArray;
			MineMaterialPackageHelper::MakeRelativeMatDirPath (StaticMesh->GetPathName (), MatDirPathArray);

			// Found all materials
            TArray<FStaticMaterial> AllMats = StaticMesh->GetStaticMaterials();

			// Find Mat by MatIndex
			FString MatchedPackagePath;
			for (uint16 MatId = 0; MatId < AllMats.Num (); ++MatId) {
				FName CurMatSlotName = AllMats[MatId].MaterialSlotName;

				// traverse all valid path to find same named material
				for (FString MatDirPath : MatDirPathArray) {
					if (!MineMaterialPackageHelper::HasFoundSlotNameMat (CurMatSlotName, MatDirPath, MatchedPackagePath)) continue;

					// Load Material to set
					UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Found Matched Package @ %s"), *MatchedPackagePath);
					UObject *const MatAsset = MinePackageLoadHelper::LoadAsset (MatchedPackagePath);
					if (!IsValid (MatAsset)) continue;

					// Cast to material type && Set new Material Interface
					UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Type as @ %s"), *MatAsset->GetClass ()->GetName ());
					auto const CurMatInterface = Cast<UMaterialInterface> (MatAsset);
					if (CurMatInterface == nullptr) continue;
					AllMats[MatId].MaterialInterface = CurMatInterface;
					break;
				}
			} // End Of Iterator of MatIds

			StaticMesh->Modify ();
			StaticMesh->SetStaticMaterials (AllMats);
			ObjectToSave.Emplace (StaticMesh);
		}
		UPackageTools::SavePackagesForObjects (ObjectToSave);
	};
};


class FAssetsProcessorFormSelection_UStaticMesh_AbcClothBindToMatSlots : public TAssetsProcessorFormSelection_Builder<UStaticMesh>
{
public:
	const bool bDebugCls = true;
	using FMaterialType = FStaticMaterial;

	FAssetsProcessorFormSelection_UStaticMesh_AbcClothBindToMatSlots () :TAssetsProcessorFormSelection_Builder<UStaticMesh> (false)
	{
		// Without SourceControl init
	}

	virtual void ProcessAssets (TArray<UStaticMesh *> &Assets) override
	{

		TArray<UObject *> ObjectToSave;
		bool const IsSourceControlValid = FAssetSourceControlHelper::IsSourceControlAvailable ();

		auto LambdaCheckIfSameMat = [&](const FString &CurrentMatPath, const TArray<FMaterialType> &AllMats)->bool {
			bool bHasFoundSamePath = false;
			// Find Mat by MatIndex
			for (uint16 MatId = 0; MatId < AllMats.Num (); ++MatId) {
				bHasFoundSamePath = AllMats[MatId].MaterialInterface->GetPathName () == CurrentMatPath;
				if (bHasFoundSamePath) {
					if (bDebugCls) UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Input : %s ; Mat Matched @ MatID %d ;Same as current setted one\n"), *CurrentMatPath, MatId);
					break;
				}
			}
			return bHasFoundSamePath;
		};

		// NOTE: Use regex to search main ruled name && material section index in cache
		// "^.*?_(.*?)\\d?(?:Shape)_(\\d?)$" match "Abc_GreenOneShape_0" <- Alembic Stream-able Track Name
		// Get $1 == "GreenOne", $2 == "0"
		auto LambdaRegexMatchShape = [&](const FString &Str, TArray<FString> &Result)->bool {
			// TODO: Should fix use string finder
			static const FRegexPattern Patten = FRegexPattern (TEXT ("^(?:.*?_)?(.*?)(?:_\\d*?)?Shape_(\\d*?)$"));
			FRegexMatcher Matcher (Patten, Str);
			Result.Empty ();
			while (Matcher.FindNext ()) {
				Result.Emplace (Matcher.GetCaptureGroup (1));
				Result.Emplace (Matcher.GetCaptureGroup (2));
				if (bDebugCls) UE_LOG (LogMineCustomToolEditor, Log, TEXT ("MatChecker Input : %s ; Regex Matched :%s @ %s \n"), *Str, *Result[0], *Result[1]);
			}
			return Result.Num () == 0 ? false : true;
		};

		auto LambdaFindMatIdByName = [&](const FString &NameString, const TArray<FMaterialType> &AllMats, uint16 &RefMatID) -> bool {
			bool bHasFoundMatchedMatId = false;
			if (bDebugCls) UE_LOG (LogMineCustomToolEditor, Log, TEXT ("MatFinder Try to find with key name : %s ;\n"), *NameString);

			TArray<FString> IgnoreKeywords = { TEXT ("_DaiLi"), TEXT ("_Proxy") };

			for (uint16 MatId = 0; MatId < AllMats.Num (); ++MatId) {

				/* NOTE: Ready for process with names */
				if (!AllMats[MatId].MaterialInterface->IsValidLowLevel ())
					continue;

				FString &&CurMatInterfaceName = AllMats[MatId].MaterialInterface->GetName ();
				FString &&CurMatSlotName = AllMats[MatId].MaterialSlotName.ToString ();
				UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Try to find with interface name :%s, matslot name :%s\n"), *CurMatInterfaceName, *CurMatSlotName);


				/* NOTE: Skip with IgnoreKeywords */
				bool &&bSkipWithIgnoreKeyword = false;
				for (const FString Keyword : IgnoreKeywords) {
					if (CurMatInterfaceName.EndsWith (Keyword) || CurMatSlotName.EndsWith (Keyword)) {
						UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Name [%s] with Proxy name, skip! \n"), *NameString);
						bSkipWithIgnoreKeyword = true;
						break;
					}
				}
				if ((bSkipWithIgnoreKeyword)) continue;


				/* NOTE: Check with current Material Interface name */
				if (CurMatInterfaceName.Find (NameString, ESearchCase::IgnoreCase, ESearchDir::FromEnd) > 0) {

					UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Mat Interface Mode; Mat Matched @ MatID %d \n"), *NameString, MatId);
					RefMatID = MatId;
					bHasFoundMatchedMatId = bHasFoundMatchedMatId || true;
				}
				/* NOTE: Check with current Material Slot name */
				if (!bHasFoundMatchedMatId) {
					UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Not found by Mat Interface Mode , using Mat SlotName Mode\n"));
					if (CurMatSlotName.Find (NameString, ESearchCase::IgnoreCase, ESearchDir::FromEnd) < 0) continue;
					else {
						UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Mat SlotName Mode; Mat Matched @ MatID %d \n"), MatId);
						RefMatID = MatId;
						bHasFoundMatchedMatId = bHasFoundMatchedMatId || true;
					}
				}
			}
			return bHasFoundMatchedMatId;
		};

		for (auto const CurrentMesh : Assets) {

			// Start job!
			auto const CurrentPathName = CurrentMesh->GetPathName ();
			UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *CurrentPathName);

			// Generate abc dir path
			TArray<FString> AbcPathArray;
			MakeRelativeAbcDirPath (CurrentPathName, AbcPathArray);

			// Found Materials
			auto AllMats = CurrentMesh->GetStaticMaterials ();


			// Traversal all Alembic Sub-Path
			for (auto AbcDirPath : AbcPathArray) {

				if (!FPaths::DirectoryExists (AbcDirPath)) continue;

				// Make sure Abc Package path is valid, Or Skip
				TArray<FString> MatchedPackagePaths;
				if (!HasFoundAnimationAbcFiles (AbcDirPath, MatchedPackagePaths))
					continue;

				for (auto const MatchedPackagePath : MatchedPackagePaths) {
					// Load Asset to UObject to modify
					UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Found Matched Package @ %s"), *MatchedPackagePath);
					UObject *const AbcAsset = MinePackageLoadHelper::LoadAsset (MatchedPackagePath);
					if (!IsValid (AbcAsset)) continue;

					// Check if abc type
					auto const GeoCache = Cast<UGeometryCache> (AbcAsset);
					if (GeoCache == nullptr || GeoCache->StaticClass ()->GetFName () != UGeometryCache::StaticClass ()->GetFName ()) {
						UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s is not valid ABC GeometryCache, SKIP\n"), *AbcAsset->GetFullName ());
						continue;
					}

					// NOTE: Get Current Geometry Materials && Tracks
					TArray<UMaterialInterface *> GeoCacheMatArray = GeoCache->Materials;
					TArray<UGeometryCacheTrack *> GeoCacheTracks = GeoCache->Tracks;
					int32 &&GeometryCacheTracksCount = GeoCacheTracks.Num ();
					auto &&GeoCacheMatCount = GeoCacheMatArray.Num ();

					// NOTE: Skip when Flattened track or no track here
					if (GeometryCacheTracksCount < 1 || GeoCacheTracks[0]->GetFName () == FName (TEXT ("Flattened_Track"))) {
						UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s is Flattened Alembic Track, SKIP!\n"), *AbcAsset->GetFullName ());
						continue;
					}

					bool bShouldModify = false;
					for (uint16 GeoCacheTrackId = 0; GeoCacheTrackId < GeometryCacheTracksCount; ++GeoCacheTrackId) {
						FString CurrentTrackName = GeoCacheTracks[GeoCacheTrackId]->GetName ();
						uint16 MatchedCurMatId = 0;
						FString MatchedCurMainName = "UnknownMesh";

						// NOTE: Found main mesh name and section index
						static TArray<FString> RegexMatchResult;
						if (LambdaRegexMatchShape (CurrentTrackName, RegexMatchResult)) {
							MatchedCurMainName = RegexMatchResult[0];
							// MatchedCurMatId = FCString::Atoi (*RegexMatchResult[1]);
							MatchedCurMatId = GeoCacheTrackId > GeoCacheMatCount ? GeoCacheMatCount : GeoCacheTrackId;
						}
						else continue;


						// NOTE: Use main string to make slot mat name
						uint16 RefMeshMatId;
						if (!LambdaFindMatIdByName (MatchedCurMainName, AllMats, RefMeshMatId))
							continue;

						// NOTE: Check if Already Material has been set
						//if (LambdaCheckIfSameMat (GeoCacheMatArray[MatchedCurMatId]->GetPathName (), AllMats))
						//    continue;
						if (GeoCacheMatArray[MatchedCurMatId]->GetPathName () == AllMats[RefMeshMatId].MaterialInterface->GetPathName ())
							continue;


						GeoCacheMatArray[MatchedCurMatId] = AllMats[RefMeshMatId].MaterialInterface;
						bShouldModify = true;
					}
					if (!bShouldModify) continue;

					// Check Out file to Modify
					if (IsSourceControlValid) {
						if (!FAssetSourceControlHelper::CheckOutFile (MatchedPackagePath)) {
							UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s Can't checkout, SKIP\n"), *AbcAsset->GetFullName ());
							continue;
						}
					}

					// Update GeometryCache Materials
					GeoCache->Modify ();
					GeoCache->Materials = GeoCacheMatArray;
					ObjectToSave.Add (AbcAsset);
				}
			}
		}
		UPackageTools::SavePackagesForObjects (ObjectToSave);
	} // End Of ProcessAssets

	/**
	 * @brief :Check if Animation Cache Abc File in inputted Abc Directory Path with config rule
	 */
	static bool HasFoundAnimationAbcFiles (const FString &AbcDirPath, TArray<FString> &MatchedPackagePaths)
	{

		// Read config to match sub-dir
		auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
		const FString &&ConfigSubPathRule =
			ConfigSettings->bUseCustomProxyConfig ?
			ConfigSettings->ConfigAlembicAnimationSubDirMatchKey : TEXT ("AnimCache");
		FString &&MatchedDirPath = FPaths::ConvertRelativePathToFull (AbcDirPath / ConfigSubPathRule);

		// Find all package name under current AnimCache Directory
		MatchedPackagePaths.Empty ();
		IFileManager::Get ().FindFiles (MatchedPackagePaths, *MatchedDirPath, TEXT ("uasset"));

		// NOTE: make fully path
		for (int PathId = 0; PathId < MatchedPackagePaths.Num (); ++PathId) {
			//PackageName = FPaths::ConvertRelativePathToFull (MatchedDirPath, PackageName);
			FPackageName::TryConvertFilenameToLongPackageName ((MatchedDirPath / MatchedPackagePaths[PathId]), MatchedPackagePaths[PathId]);
		}

		return MatchedPackagePaths.Num () > 0 ? true : false;
	}

	static void MakeRelativeAbcDirPath (const FString &MatPackagePath, TArray<FString> &AbcPathArray)
	{
		AbcPathArray.Empty ();
		auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();
		FString const ConfigAlembicPathRule =
			ConfigSettings->bUseCustomProxyConfig ?
			ConfigSettings->ConfigAlembicAnimCachePathRule :
			TEXT ("Animations/Alembic");

		// Path @ [CurrentAssetDir]/Animations/Alembic
		FString TempDirPath;
		FPackageName::TryConvertLongPackageNameToFilename (MatPackagePath, TempDirPath);
		TempDirPath = FPaths::GetPath (TempDirPath) / ConfigAlembicPathRule;
		UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to found all Alembic Diectory @ %s"), *TempDirPath);
		if (FPaths::DirectoryExists (TempDirPath)) {
            FMeshAssetPathFounder_Internal::FDirectoryVisitor Visitor;
			FPlatformFileManager::Get ().GetPlatformFile ().IterateDirectory (*TempDirPath, Visitor);
			AbcPathArray = Visitor.DirectoryArray;
		}
	}

};


//////////////////////////////////////////////////////////////////////////
// FMineContentBrowserExtensions_UStaticMesh

class FMineContentBrowserExtensions_UStaticMesh
{

public:

    ~FMineContentBrowserExtensions_UStaticMesh () {};

	static void ExecuteSelectedContentFunctor (
		TSharedPtr<FAssetsProcessorFormSelection_Base> SelectedAssetFunctor
	)
	{
		SelectedAssetFunctor->Execute ();
	}

	/**
	 * @brief Add Content Based Menu ! Remember using Delegate on loading time hook!
	 * This Function help to check if current type should create FExtender.
	 * @param SelectedAssets Selected Assets that would reference from content browser
	 * @return shareRef to FExtender
	 */
	static  TSharedRef<FExtender>
		OnExtendContentBrowserAssetSelectionMenu (
			const TArray<FAssetData> &SelectedAssets
		)
	{
		// UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("On StaticMesh Asset!"));
		TSharedRef<FExtender> Extender (new FExtender ());

		// Run thru the assets to determine if any meet our criteria
		bool bCurrentType = false;
		for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
			const FAssetData &Asset = *AssetIt;
			bCurrentType = bCurrentType || (Asset.GetAsset()->GetClass()->GetFName() == UStaticMesh::StaticClass ()->GetFName ());
		}

		if (bCurrentType) {
			// Add the Static actions sub-menu extender
				Extender->AddMenuExtension (
					"GetAssetActions",
					EExtensionHook::After,
					nullptr,
					FMenuExtensionDelegate::CreateStatic (
						&FMineContentBrowserExtensions_UStaticMesh::CreateActionsSubMenu,
						SelectedAssets)
				);
		}

		return Extender;
	}

    static void CreateActionsSubMenu (
		FMenuBuilder &MenuBuilder,
		TArray<FAssetData> SelectedAssets
	)
	{
		const FSlateIcon BaseMenuIcon = FSlateIcon ();
		const FText BaseMenuName = LOCTEXT ("ActionsSubMenuLabel", "Mine StaticMesh Actions");
		const FText BaseMenuTip = LOCTEXT ("ActionsSubMenuToolTip", "Type-related actions for Static Mesh Asset.");

		MenuBuilder.AddSubMenu (
			BaseMenuName,
			BaseMenuTip,
			FNewMenuDelegate::CreateStatic (&FMineContentBrowserExtensions_UStaticMesh::PopulateActionsMenu, SelectedAssets),
			false,
			BaseMenuIcon,
			true,
			FName (TEXT ("MineStaticMeshActions"))
		);
	}

	static  void PopulateActionsMenu (
		FMenuBuilder &MenuBuilder,
		TArray<FAssetData> SelectedAssets
	) 
	{

		//////////////////////////////////////////////////////////////
		///
		///	FAssetsProcessorFormSelection_UStaticMesh_SetMeshDF
	    {
	        TSharedPtr<FAssetsProcessorFormSelection_UStaticMesh_SetMeshDF> const StaticMeshPropsProcessor =
                MakeShareable (new FAssetsProcessorFormSelection_UStaticMesh_SetMeshDF);

	        // Add current selection to AssetsProcessor
	        StaticMeshPropsProcessor->SelectedAssets = SelectedAssets;

	        // Build a Action Struct : ExecuteSelectedContentFunctor(AssetsProcessor);AssetsProcessor->Execute ();
	        FUIAction const Action_AutoSetProps_ProcessFromAssets (
                FExecuteAction::CreateStatic (
                    &FMineContentBrowserExtensions_UStaticMesh::ExecuteSelectedContentFunctor,
                    StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (StaticMeshPropsProcessor))
            );

	        // Add to Menu
	        MenuBuilder.AddMenuEntry (
                LOCTEXT ("CBE_StaticMesh_BuildDF", "Build DistanceField"),
                LOCTEXT ("CBE_StaticMesh_BuildDF_ToolTips", "Build DistanceField from selected"),
                FSlateIcon (),
                Action_AutoSetProps_ProcessFromAssets,
                NAME_None,
                EUserInterfaceActionType::Button);
	    }

		//////////////////////////////////////////////////////////////
		///
		///	FAssetsProcessorFormSelection_UStaticMesh_RemoveDF
	    {
	        TSharedPtr<FAssetsProcessorFormSelection_UStaticMesh_RemoveDF> const StaticMeshRmDfProcessor =
               MakeShareable (new FAssetsProcessorFormSelection_UStaticMesh_RemoveDF);

	        // Add current selection to AssetsProcessor
	        StaticMeshRmDfProcessor->SelectedAssets = SelectedAssets;

	        //StaticMeshRmDfProcessor->SelectedAssets = SelectedAssets;

	        // Build a Action Struct : ExecuteSelectedContentFunctor(AssetsProcessor);AssetsProcessor->Execute ();
	        FUIAction const Action_RmDF_ProcessFromAssets (
                FExecuteAction::CreateStatic (
                    &FMineContentBrowserExtensions_UStaticMesh::ExecuteSelectedContentFunctor,
                    StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (StaticMeshRmDfProcessor))
            );

	        // Add to Menu
	        MenuBuilder.AddMenuEntry (
                LOCTEXT ("CBE_StaticMesh_RmDF", "Remove DistanceField"),
                LOCTEXT ("CBE_StaticMesh_RmDF_ToolTips", "Remove DistanceField from selected"),
                FSlateIcon (),
                Action_RmDF_ProcessFromAssets,
                NAME_None,
                EUserInterfaceActionType::Button);
	    }


		//////////////////////////////////////////////////////////////
		///
		///	FAssetsProcessorFormSelection_UStaticMesh_SetMeshHighPrecision
	    {
	        TSharedPtr<FAssetsProcessorFormSelection_UStaticMesh_SetMeshHighPrecision> const StaticMeshSetMeshHighPrecisionProcessor =
               MakeShareable (new FAssetsProcessorFormSelection_UStaticMesh_SetMeshHighPrecision);

	        // Add current selection to AssetsProcessor
	        StaticMeshSetMeshHighPrecisionProcessor->SelectedAssets = SelectedAssets;

	        // Build a Action Struct : ExecuteSelectedContentFunctor(AssetsProcessor);AssetsProcessor->Execute ();
	        FUIAction const Action_SetMeshHighPrecision_ProcessFromAssets (
                FExecuteAction::CreateStatic (
                    &FMineContentBrowserExtensions_UStaticMesh::ExecuteSelectedContentFunctor,
                    StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (StaticMeshSetMeshHighPrecisionProcessor))
            );

	        // Add to Menu
	        MenuBuilder.AddMenuEntry (
                LOCTEXT ("CBE_StaticMesh_SetMeshHighPrecision", "Set HighPrecision UV"),
                LOCTEXT ("CBE_StaticMesh_SetMeshHighPrecision_ToolTips", "Set HighPrecision UV for UIDM Virtual Texture to remove cracked lines"),
                FSlateIcon (),
                Action_SetMeshHighPrecision_ProcessFromAssets,
                NAME_None,
                EUserInterfaceActionType::Button);
	    }


		//////////////////////////////////////////////////////////////
		///
		///	FAssetsProcessorFormSelection_UStaticMesh_AutoBindMat
		{
			TSharedPtr<FAssetsProcessorFormSelection_UStaticMesh_AutoBindMat> const StaticMeshAutoBindMatProcessor =
				MakeShareable (new FAssetsProcessorFormSelection_UStaticMesh_AutoBindMat);

			// Add current selection to AssetsProcessor
			StaticMeshAutoBindMatProcessor->SelectedAssets = SelectedAssets;

			// Build a Action Struct : ExecuteSelectedContentFunctor(AssetsProcessor);AssetsProcessor->Execute ();
			FUIAction const Action_AutoBindMat_ProcessFromAssets (
				FExecuteAction::CreateStatic (
					&FMineContentBrowserExtensions_UStaticMesh::ExecuteSelectedContentFunctor,
					StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (StaticMeshAutoBindMatProcessor))
			);

			// Add to Menu
			MenuBuilder.AddMenuEntry (
				LOCTEXT ("CBE_StaticMesh_AutoBindMat", "Auto Bind Material"),
				LOCTEXT ("CBE_StaticMesh_AutoBindMat_ToolTips", "Auto bind material from selected"),
				FSlateIcon (),
				Action_AutoBindMat_ProcessFromAssets,
				NAME_None,
				EUserInterfaceActionType::Button);
		}


		//////////////////////////////////////////////////////////////
		///
		///	FAssetsProcessorFormSelection_UStaticMesh_AbcClothBindToMatSlots
	    {
			TSharedPtr<FAssetsProcessorFormSelection_UStaticMesh_AbcClothBindToMatSlots> const StaticMeshAbcClothBindToMatSlots =
				MakeShareable (new FAssetsProcessorFormSelection_UStaticMesh_AbcClothBindToMatSlots);

			// Add current selection to AssetsProcessor
			StaticMeshAbcClothBindToMatSlots->SelectedAssets = SelectedAssets;

			// Build a Action Struct : ExecuteSelectedContentFunctor(AssetsProcessor);AssetsProcessor->Execute ();
			FUIAction const Action_AbcClothBindToMatSlots_ProcessFromAssets (
				FExecuteAction::CreateStatic (
					&FMineContentBrowserExtensions_UStaticMesh::ExecuteSelectedContentFunctor,
					StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (StaticMeshAbcClothBindToMatSlots))
			);

			// Add to Menu
			MenuBuilder.AddMenuEntry (
				LOCTEXT ("CBE_StaticMesh_AutoBindAbcCacheMat", "Bind Abc-AnimCache Material"),
				LOCTEXT ("CBE_StaticMesh_AutoBindAbcCacheMat_ToolTips", "Auto Bind Abc-AnimCache Material from selected"),
				FSlateIcon (),
				Action_AbcClothBindToMatSlots_ProcessFromAssets,
				NAME_None,
				EUserInterfaceActionType::Button);
	    }

		//////////////////////////////////////////////////////////////
		///
		/// End
	}

};




//////////////////////////////////////////////////////////////////////////
// FStaticMeshMenuActionsListener


void FStaticMeshMenuActionsListener::InstallHooks ()
{
	//TSharedPtr<FMineContentBrowserExtensions_UStaticMesh> CB_Extension_UStaticMesh =
	//	MakeShareable (new FMineContentBrowserExtensions_UStaticMesh);

	UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install UStaticMesh Asset Menu Hook"));
	// Declare Delegate 
	ContentBrowserExtenderDelegate =
		FContentBrowserMenuExtender_SelectedAssets::CreateStatic(
			&FMineContentBrowserExtensions_UStaticMesh::OnExtendContentBrowserAssetSelectionMenu);

	// Get all content module delegates
	TArray<FContentBrowserMenuExtender_SelectedAssets>
    &CBMenuExtenderDelegates = GetExtenderDelegates ();

	// Add The delegate of mine
	CBMenuExtenderDelegates.Add (ContentBrowserExtenderDelegate);
	ContentBrowserExtenderDelegateHandle = 
		CBMenuExtenderDelegates.Last ().GetHandle ();
}

void FStaticMeshMenuActionsListener::RemoveHooks ()
{
	TArray<FContentBrowserMenuExtender_SelectedAssets>
    &CBMenuExtenderDelegates = GetExtenderDelegates ();

	CBMenuExtenderDelegates.RemoveAll (
		[&](const FContentBrowserMenuExtender_SelectedAssets &Delegate)->bool
		{
		    return Delegate.GetHandle () == ContentBrowserExtenderDelegateHandle;
		}
	);
}

TArray<FContentBrowserMenuExtender_SelectedAssets>& 
FStaticMeshMenuActionsListener::GetExtenderDelegates()
{
	/////////////////////////////
	///Get ContentBrowser Module
	/////////////////////////////
	FContentBrowserModule &ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule> (TEXT ("ContentBrowser"));

	/////////////////////////////
	///Get Selection based Extender Delegate Event Source
	/////////////////////////////

    return ContentBrowserModule.GetAllAssetViewContextMenuExtenders ();
}

//////////////////////////////////////////////////////////////////////////
// End LocText NameSpace
#undef LOCTEXT_NAMESPACE
