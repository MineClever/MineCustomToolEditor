#include "AssetMenuTools/FStaticMeshMenuActionsListener.h"
#include "AssetMenuTools/TAssetsProcessorFormSelection.hpp"
#include "AssetCreateHelper/FMinePackageSaveHelper.h"
#include "AssetCreateHelper/FMinePackageToObjectHelper.hpp"
#include "ConfigIO/ConfigIO.h"
#include "Internationalization/Regex.h"
#include "AssetMenuTools/FSkeletalMeshActionListener.h"

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


	FAssetsProcessorFormSelection_UStaticMesh_AbcClothBindToMatSlots () :TAssetsProcessorFormSelection_Builder<UStaticMesh> (false)
	{
		// Without SourceControl init
	}

	virtual void ProcessAssets (TArray<UStaticMesh *> &Assets) override
	{
        #define MACRO_MAKE_RELATIVE_ABC_DIR_PATH FSkeletalMeshProcessor_AutoSet_Internal::FSkeletalMeshProcessor_AbcClothBindToMatSlots::MakeRelativeAbcDirPath
        #define MACRO_HAS_FOUND_CLOTH_ABC_FILE FSkeletalMeshProcessor_AutoSet_Internal::FSkeletalMeshProcessor_AbcClothBindToMatSlots::HasFoundClothAbcFile

		TArray<UObject *> ObjectToSave;
		bool const IsSourceControlValid = FAssetSourceControlHelper::IsSourceControlAvailable ();

		for (auto MeshIterator = Assets.CreateConstIterator (); MeshIterator; ++MeshIterator) {

			// Start job!
			auto const CurrentMesh = *MeshIterator;
			UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *CurrentMesh->GetPathName ());

			// Generate abc dir path
			TArray<FString> AbcPathArray;
			MACRO_MAKE_RELATIVE_ABC_DIR_PATH (CurrentMesh->GetPathName (), AbcPathArray);

			// Found Materials
			TArray<FStaticMaterial> AllMats = CurrentMesh->GetStaticMaterials ();

			// Find Mat by MatIndex
			for (uint16 MatId = 0; MatId < AllMats.Num (); ++MatId) {
				FName CurMatSlotName = AllMats[MatId].MaterialSlotName;
				// Traversal all Alembic Sub-Path
				for (auto AbcDirPath : AbcPathArray) {
					// UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Searching @ %s"), *AbcDirPath);
					if (!FPaths::DirectoryExists (AbcDirPath)) continue;

					FString MatchedPackagePath;
					// Make sure SlotNamed Abc Package path is valid, Or Skip
					if (!MACRO_HAS_FOUND_CLOTH_ABC_FILE (CurMatSlotName, AbcDirPath, MatchedPackagePath))
						continue;

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

					// Get Current Geometry Materials
					TArray<UMaterialInterface *> GeoCacheMatArray = GeoCache->Materials;

					// Check if valid Mat on GeometryCache
					if (GeoCacheMatArray.Num () > 0) {
						// Check if Already Material has been set
						if (GeoCacheMatArray[0]->GetPathName () == AllMats[MatId].MaterialInterface->GetPathName ())
							continue;
					}
					else continue;

					// Check Out file to Modify
					if (IsSourceControlValid) {
						if (!FAssetSourceControlHelper::CheckOutFile (MatchedPackagePath)) {
							UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s Can't checkout, SKIP\n"), *AbcAsset->GetFullName ());
							continue;
						}
					}

					// Replace Current Mat
					GeoCache->Modify ();
					for (uint16 GeoMatId = 0; GeoMatId < GeoCacheMatArray.Num (); ++GeoMatId) {
						GeoCacheMatArray[GeoMatId] = AllMats[MatId].MaterialInterface;
					}

					// Update GeometryCache Materials
					GeoCache->Materials = GeoCacheMatArray;
					ObjectToSave.Add (AbcAsset);
				} // Terminate Traversal AbcPathArray
			} // End Of Iterator of MatIds
		} // End of Iterator Of Assets
		UPackageTools::SavePackagesForObjects (ObjectToSave);
	} // End Of ProcessAssets
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
			bCurrentType = bCurrentType || (Asset.AssetClass == UStaticMesh::StaticClass ()->GetFName ());
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
