#pragma once
#include "MineMouduleDefine.h"
#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "GeometryCache.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"
#include <AssetMenuTools/TMineContentBrowserExtensions_SelectedAssets_Base.h>



namespace FAlembicAssetProcessor_Internal
{
	using LocAssetType = UGeometryCache;
}


namespace FAlembicAssetActionsMenuCommandsInfo_Internal
{
	class MineAssetCtxMenuCommandsInfo;
}


namespace FAlembicAssetMenuActionsListener_Internal
{
	using namespace FAlembicAssetProcessor_Internal;
	using namespace TMineContentBrowserExtensions_SelectedAssets_Internal;
	using namespace FAlembicAssetActionsMenuCommandsInfo_Internal;

	class FSkeletalMeshMenuActionsListener : public IMineCustomToolModuleListenerInterface
	{

	public:
		virtual ~FSkeletalMeshMenuActionsListener () override {};

		virtual void OnStartupModule () override
		{
			this->InstallHooks ();
		};

		virtual void OnShutdownModule () override
		{
			this->RemoveHooks ();
		};

		void InstallHooks ();

		void RemoveHooks () const
		{
			TArray<FContentBrowserMenuExtender_SelectedAssets>
				&CBMenuExtenderDelegates = GetExtenderDelegates ();

			CBMenuExtenderDelegates.RemoveAll (
				[&](const FContentBrowserMenuExtender_SelectedAssets &Delegate)->bool {
					return Delegate.GetHandle () == ContentBrowserExtenderDelegateHandle;
				}
			);
		};

		static TArray<FContentBrowserMenuExtender_SelectedAssets> &GetExtenderDelegates ()
		{
			FContentBrowserModule &ContentBrowserModule =
				FModuleManager::LoadModuleChecked<FContentBrowserModule> (TEXT ("ContentBrowser"));

			return ContentBrowserModule.GetAllAssetViewContextMenuExtenders ();
		};

	private:
		FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
		FDelegateHandle ContentBrowserExtenderDelegateHandle;
		static TSharedRef<TMineContentBrowserExtensions_SelectedAssets_Base<LocAssetType, MineAssetCtxMenuCommandsInfo>> MenuExtension;

	};

}
