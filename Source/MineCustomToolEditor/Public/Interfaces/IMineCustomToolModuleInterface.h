#pragma once
#include "Modules/ModuleManager.h"

class IMineCustomToolModuleListenerInterface
{
public:
    virtual void OnStartupModule () {};
    virtual void OnShutdownModule () {};
};

class IMineCustomToolModuleInterface : public IModuleInterface
{
public:
    void StartupModule () override
    {
        if (!IsRunningCommandlet ()) {
            AddModuleListeners ();
            for (auto &&Listener : ModuleListeners) {
                Listener->OnStartupModule ();
            }
        }
    }

    void ShutdownModule () override
    {
        for (auto &&Listener : ModuleListeners)
        {
            Listener->OnShutdownModule ();
        }
    }

    virtual void AddModuleListeners () {};

protected:
    TArray<TSharedRef<IMineCustomToolModuleListenerInterface>> ModuleListeners;
};