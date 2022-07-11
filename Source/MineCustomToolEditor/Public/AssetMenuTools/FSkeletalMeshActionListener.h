#pragma once
#include "MineMouduleDefine.h"
#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"


class FSkeletalMeshMenuActionsListener : public IMineCustomToolModuleListenerInterface
{
public:
	virtual void OnStartupModule () override
	{
		this->InstallHooks ();
	};
	virtual void OnShutdownModule () override
	{
		this->RemoveHooks ();
	};
	void InstallHooks ();
	void RemoveHooks ();
	virtual ~FSkeletalMeshMenuActionsListener () override {};
	static TArray<FContentBrowserMenuExtender_SelectedAssets> &GetExtenderDelegates ()
	{
		/////////////////////////////
		///Get ContentBrowser Module
		/////////////////////////////
		FContentBrowserModule &ContentBrowserModule =
			FModuleManager::LoadModuleChecked<FContentBrowserModule> (TEXT ("ContentBrowser"));

		return ContentBrowserModule.GetAllAssetViewContextMenuExtenders ();
	};
public:
	FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
	FDelegateHandle ContentBrowserExtenderDelegateHandle;
};
