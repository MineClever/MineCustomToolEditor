#pragma once
#include "MineMouduleDefine.h"
#include "CoreMinimal.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"


class StaticMeshMenuActionsListener : public IMineCustomToolModuleListenerInterface
{
public:
	virtual void OnStartupModule () override;
	virtual void OnShutdownModule () override;
	static void InstallHooks ();
	static void RemoveHooks ();
	static TArray<FContentBrowserMenuExtender_SelectedAssets> &GetExtenderDelegates ();
public:
	static FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
	static FDelegateHandle ContentBrowserExtenderDelegateHandle;
};
