#include "AssetMenuTools/StaticMeshMenuActionsListener.h"
#include "AssetMenuTools/FAssetsProcessorFormSelection.hpp"

//////////////////////////////////////////////////////////////////////////
// Start LocText NameSpace
#define LOCTEXT_NAMESPACE "StaticMeshMenuActionsListener"



//////////////////////////////////////////////////////////////////////////
// TAssetsProcessorFormSelection_UStaticMesh

class FAssetsProcessorFormSelection_UStaticMesh_PrintName : public TAssetsProcessorFormSelection_Builder<UStaticMesh>
{
public:

	virtual void ProcessAssets (TArray<UStaticMesh *> &Assets) override
	{
		UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Static Mesh Base Processor Run !! "));
	};

};

class FAssetsProcessorFormSelection_UStaticMesh_BuildDF : public TAssetsProcessorFormSelection_Builder<UStaticMesh>
{
public:

	virtual void ProcessAssets (TArray<UStaticMesh *> &Assets) override
	{
		UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Static Mesh Build Distance Filed Processor Run !! "));
	};

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
		UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("On StaticMesh Asset!"));
		TSharedRef<FExtender> Extender (new FExtender ());

		// Run thru the assets to determine if any meet our criteria
		bool bCurrentType = false;
		for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
			const FAssetData &Asset = *AssetIt;
			bCurrentType = bCurrentType || (Asset.AssetClass == UStaticMesh::StaticClass ()->GetFName ());
		}

		if (bCurrentType) {
			// Add the sprite actions sub-menu extender
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
		const FText BaseMenuName = LOCTEXT ("ActionsSubMenuLabel", "Static Mesh Actions");
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
		///	FAssetsProcessorFormSelection_UStaticMesh_PrintName

	    TSharedPtr<FAssetsProcessorFormSelection_UStaticMesh_PrintName> StaticMeshBaseProcessor =
			MakeShareable(new FAssetsProcessorFormSelection_UStaticMesh_PrintName);

		// Add current selection to AssetsProcessor
		StaticMeshBaseProcessor->SelectedAssets = SelectedAssets;

		// Build a Action Struct : ExecuteSelectedContentFunctor(AssetsProcessor);AssetsProcessor->Execute ();
		FUIAction Action_PrintName_ProcessFromAssets (
			FExecuteAction::CreateStatic (
				&FMineContentBrowserExtensions_UStaticMesh::ExecuteSelectedContentFunctor,
				StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (StaticMeshBaseProcessor))
		);

		// Add to Menu
		MenuBuilder.AddMenuEntry (
			LOCTEXT ("CBE_StaticMesh_PrintName", "Auto Get All Name"),
			LOCTEXT ("CBE_StaticMesh_PrintName_ToolTips", "Auto Get All Name from selected"),
			FSlateIcon (),
			Action_PrintName_ProcessFromAssets,
			NAME_None,
			EUserInterfaceActionType::Button);

		//////////////////////////////////////////////////////////////
		///
		///	FAssetsProcessorFormSelection_UStaticMesh_BuildDF

		TSharedPtr<FAssetsProcessorFormSelection_UStaticMesh_BuildDF> StaticMeshDFProcessor =
			MakeShareable (new FAssetsProcessorFormSelection_UStaticMesh_BuildDF);

		// Add current selection to AssetsProcessor
		StaticMeshDFProcessor->SelectedAssets = SelectedAssets;

		// Build a Action Struct : ExecuteSelectedContentFunctor(AssetsProcessor);AssetsProcessor->Execute ();
		FUIAction Action_BuidDF_ProcessFromAssets (
			FExecuteAction::CreateStatic (
				&FMineContentBrowserExtensions_UStaticMesh::ExecuteSelectedContentFunctor,
				StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (StaticMeshDFProcessor))
		);

		// Add to Menu
		MenuBuilder.AddMenuEntry (
			LOCTEXT ("CBE_StaticMesh_BuildDF", "Auto Build DistanceField"),
			LOCTEXT ("CBE_StaticMesh_BuildDF_ToolTips", "Auto Build DistanceField from selected"),
			FSlateIcon (),
			Action_BuidDF_ProcessFromAssets,
			NAME_None,
			EUserInterfaceActionType::Button);

	}

};



//////////////////////////////////////////////////////////////////////////
// StaticMeshMenuActionsListener

void StaticMeshMenuActionsListener::OnStartupModule()
{
	InstallHooks ();
}

void StaticMeshMenuActionsListener::OnShutdownModule()
{
	RemoveHooks ();
}

void StaticMeshMenuActionsListener::InstallHooks ()
{
	TSharedPtr<FMineContentBrowserExtensions_UStaticMesh> CB_Extension_UStaticMesh =
		MakeShareable (new FMineContentBrowserExtensions_UStaticMesh);
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

void StaticMeshMenuActionsListener::RemoveHooks ()
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
StaticMeshMenuActionsListener::GetExtenderDelegates()
{
	/////////////////////////////
	///Get ContentBrowser Module
	/////////////////////////////
	FContentBrowserModule &ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule> (TEXT ("ContentBrowser"));

	/////////////////////////////
	///Get Selection based Extender Delegate Event Source
	/////////////////////////////
	// TArray<FContentBrowserMenuExtender_SelectedAssets> &CBMenuExtenderDelegates =
    return ContentBrowserModule.GetAllAssetViewContextMenuExtenders ();
}

//////////////////////////////////////////////////////////////////////////
// End LocText NameSpace
#undef LOCTEXT_NAMESPACE
