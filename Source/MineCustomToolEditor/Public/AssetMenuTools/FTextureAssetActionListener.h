#pragma once
#include "MineMouduleDefine.h"
#include "CoreMinimal.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"

class FTextureAssetActionListener : public IMineCustomToolModuleListenerInterface
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
	virtual ~FTextureAssetActionListener () {};
	static TArray<FContentBrowserMenuExtender_SelectedAssets> &GetExtenderDelegates ();
public:
	FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
	FDelegateHandle ContentBrowserExtenderDelegateHandle;
};
