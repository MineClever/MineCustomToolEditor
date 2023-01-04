#pragma once
#include "MineMouduleDefine.h"
#include "CoreMinimal.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Engine/StaticMesh.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"


class FStaticMeshMenuActionsListener : public IMineCustomToolModuleListenerInterface
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
    virtual ~FStaticMeshMenuActionsListener () override {};
	static TArray<FContentBrowserMenuExtender_SelectedAssets> &GetExtenderDelegates ();
public:
    FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
    FDelegateHandle ContentBrowserExtenderDelegateHandle;
};
