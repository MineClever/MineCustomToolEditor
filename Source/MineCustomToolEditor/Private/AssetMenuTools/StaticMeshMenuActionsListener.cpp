#include "AssetMenuTools/StaticMeshMenuActionsListener.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Engine/StaticMesh.h"

//////////////////////////////////////////////////////////////////////////
// Start LocText NameSpace
#define LOCTEXT_NAMESPACE "StaticMeshMenuActionsListener"

//////////////////////////////////////////////////////////////////////////
// FContentBrowserSelectedAssetExtensionBase

class FContentBrowserSelectedAssetExtensionBase
{
public:
	TArray<struct FAssetData> SelectedAssets;

public:
	virtual void Execute () {}
	virtual ~FContentBrowserSelectedAssetExtensionBase () {}
};

//////////////////////////////////////////////////////////////////////////
// FAssetsProcessorFormSelectionBase
template<class TAsset>
class FAssetsProcessorFormSelectionBase : public FContentBrowserSelectedAssetExtensionBase
{
public:
	bool bSpecificAssetType;

	FAssetsProcessorFormSelectionBase (): bSpecificAssetType (false)
	{
	}

	virtual void ProcessAssets (TArray<TAsset *> &Meshes)
	{
	}

	virtual void Execute () override
	{
		// Filter for specific type assets
		TArray<TAsset *> Assets;
		for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
			const FAssetData &AssetData = *AssetIt;
			if (TAsset *Asset = Cast<TAsset> (AssetData.GetAsset ())) {
				Assets.Add (Asset);
			}
		}

		ProcessAssets (Assets);
	}
};

//////////////////////////////////////////////////////////////////////////
// FAssetsProcessorFormSelectionStaticMesh

class FAssetsProcessorFormSelectionStaticMesh : public FAssetsProcessorFormSelectionBase<UStaticMesh>
{
public:
	FAssetsProcessorFormSelectionStaticMesh () : FAssetsProcessorFormSelectionBase<UStaticMesh> () {};
	virtual void ProcessAssets (TArray<UStaticMesh *> &Meshes) override
	{
	    
	};
};

//////////////////////////////////////////////////////////////////////////
// FMineContentBrowserExtensions_Base

template <class TAsset>
class FMineContentBrowserExtensions_Base
{
protected:
	static FORCEINLINE FSlateIcon BaseMenuIcon = 
		FSlateIcon (FEditorStyle::GetStyleSetName (), "ClassIcon.PaperSprite");
	static FORCEINLINE FText BaseMenuName = LOCTEXT ("ActionsSubMenuLabel", "Base Actions");
	static FORCEINLINE FText BaseMenuTip = LOCTEXT ("ActionsSubMenuToolTip", "Type-related actions for this Asset.");

public:
	virtual ~FMineContentBrowserExtensions_Base () {};

	static void ExecuteSelectedContentFunctor (
		TSharedPtr<FContentBrowserSelectedAssetExtensionBase> SelectedAssetFunctor
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
	static TSharedRef<FExtender>
		OnExtendContentBrowserAssetSelectionMenu (
			const TArray<FAssetData> &SelectedAssets
		)
	{
		TSharedRef<FExtender> Extender (new FExtender ());

		// Run thru the assets to determine if any meet our criteria
		bool bCurrentType = false;
		for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
			const FAssetData &Asset = *AssetIt;
			bCurrentType = bCurrentType || (Asset.AssetClass == TAsset::StaticClass ()->GetFName ());
		}

		if (bCurrentType) {
			// Add the sprite actions sub-menu extender
			Extender->AddMenuExtension (
				"GetAssetActions",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateStatic (
					&FMineContentBrowserExtensions_Base::CreateActionsSubMenu,
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
		MenuBuilder.AddSubMenu (
			BaseMenuName,
			BaseMenuTip,
			FNewMenuDelegate::CreateRaw (&FMineContentBrowserExtensions_Base::PopulateActionsMenu, SelectedAssets),
			false,
			BaseMenuIcon
		);
	}

    virtual void PopulateActionsMenu (
		FMenuBuilder &MenuBuilder,
		TArray<FAssetData> SelectedAssets
	)
	{

	}

};

//////////////////////////////////////////////////////////////////////////
// FMineContentBrowserExtensions_Impl

class FMineContentBrowserExtensions_Impl
{
public:
	static void ExecuteSelectedContentFunctor (
		TSharedPtr<FContentBrowserSelectedAssetExtensionBase> SelectedAssetFunctor
	)
	{
		SelectedAssetFunctor->Execute ();
	}

	/**
	 * @brief Add Content Based Menu ! Remember using Delegate on loading time hook!
	 * @param SelectedAssets Selected Assets that would reference from content browser
	 * @return shareRef to FExtender
	 */
	static TSharedRef<FExtender>
		OnExtendContentBrowserAssetSelectionMenu (
			const TArray<FAssetData> &SelectedAssets
		)
	{
		TSharedRef<FExtender> Extender (new FExtender ());

		// Run thru the assets to determine if any meet our criteria
		bool bAnyTextures = false;
		for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
			const FAssetData &Asset = *AssetIt;
			bAnyTextures = bAnyTextures || (Asset.AssetClass == UTexture2D::StaticClass ()->GetFName ());
		}

		if (bAnyTextures) {
			// Add the sprite actions sub-menu extender
			Extender->AddMenuExtension (
				"GetAssetActions",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateStatic (&FMineContentBrowserExtensions_Impl::CreateActionsSubMenu, SelectedAssets));
		}

		return Extender;
	}

	static void CreateActionsSubMenu (
		FMenuBuilder &MenuBuilder,
		TArray<FAssetData> SelectedAssets
	)
	{
		MenuBuilder.AddSubMenu (
			LOCTEXT ("SpriteActionsSubMenuLabel", "Sprite Actions"),
			LOCTEXT ("SpriteActionsSubMenuToolTip", "Sprite-related actions for this texture."),
			FNewMenuDelegate::CreateStatic (&FMineContentBrowserExtensions_Impl::PopulateActionsMenu, SelectedAssets),
			false,
			FSlateIcon (FEditorStyle::GetStyleSetName (), "ClassIcon.PaperSprite")
		);
	}

	static void PopulateActionsMenu (
		FMenuBuilder &MenuBuilder, 
		TArray<FAssetData> SelectedAssets
	)
	{
		// Create sprites
		TSharedPtr<FAssetsProcessorFormSelectionStaticMesh> SpriteCreatorFunctor =
			MakeShareable (new FAssetsProcessorFormSelectionStaticMesh ());
		SpriteCreatorFunctor->SelectedAssets = SelectedAssets;

		// Build a Struct
		FUIAction Action_CreateSpritesFromTextures (
			FExecuteAction::CreateStatic (
				&FMineContentBrowserExtensions_Impl::ExecuteSelectedContentFunctor,
				StaticCastSharedPtr<FContentBrowserSelectedAssetExtensionBase> (SpriteCreatorFunctor))
		);


		MenuBuilder.AddMenuEntry (
			LOCTEXT ("CB_Extension_Texture_CreateSprite", "Create Sprite"),
			LOCTEXT ("CB_Extension_Texture_CreateSprite_Tooltip", "Create sprites from selected textures"),
			FSlateIcon (),
			Action_CreateSpritesFromTextures,
			NAME_None,
			EUserInterfaceActionType::Button);

		// Extract Sprites
		TSharedPtr<FAssetsProcessorFormSelectionStaticMesh>
	    SpriteExtractorFunctor = 
			MakeShareable (new FAssetsProcessorFormSelectionStaticMesh ());

		SpriteExtractorFunctor->SelectedAssets = SelectedAssets;
		SpriteExtractorFunctor->bSpecificAssetType = true;

		FUIAction Action_ExtractSpritesFromTextures (
			FExecuteAction::CreateStatic (
				&FMineContentBrowserExtensions_Impl::ExecuteSelectedContentFunctor,
				StaticCastSharedPtr<FContentBrowserSelectedAssetExtensionBase> (SpriteExtractorFunctor))
		);

		MenuBuilder.AddMenuEntry (
			LOCTEXT ("CB_Extension_Texture_ExtractSprites", "Extract Sprites"),
			LOCTEXT ("CB_Extension_Texture_ExtractSprites_Tooltip", "Extract sprites from selected textures"),
			FSlateIcon (),
			Action_ExtractSpritesFromTextures,
			NAME_None,
			EUserInterfaceActionType::Button);
	}



};

class FMineContentBrowserExtensions_StaticMesh : public FMineContentBrowserExtensions_Base<UStaticMesh>
{
public:
	static FSlateIcon BaseMenuIcon = FSlateIcon ();
	static FText BaseMenuName = LOCTEXT ("ActionsSubMenuLabel", "Static Mesh Actions");
	static FText BaseMenuTip = LOCTEXT ("ActionsSubMenuToolTip", "Type-related actions for Static Mesh Asset.");


	virtual void PopulateActionsMenu (
		FMenuBuilder &MenuBuilder,
		TArray<FAssetData> SelectedAssets
	) override
	{
	    
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
	// Declare Delegate 
	ContentBrowserExtenderDelegate = 
		FContentBrowserMenuExtender_SelectedAssets::CreateStatic (
			&FMineContentBrowserExtensions_Impl::OnExtendContentBrowserAssetSelectionMenu
		);

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
		[](const FContentBrowserMenuExtender_SelectedAssets &Delegate)->bool
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
