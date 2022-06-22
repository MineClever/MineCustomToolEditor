#pragma once
#include "MineMouduleDefine.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"


class FLayerSelectionMenuActionsListener : public IMineCustomToolModuleListenerInterface
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
    virtual ~FLayerSelectionMenuActionsListener () {};

public:

};
