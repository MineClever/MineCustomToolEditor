#pragma once
#include "MineCustomToolEditor.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

/*
 * The base class is also inherit from IModuleListenerInterface.
 * In OnStartupModule we register a tab, and unregister it in OnShutdownModule.
 * Then in MakeMenuEntry,
 * we let FGlobalTabmanager to populate tab for this menu item.
 * We leave SpawnTab function to be overriden by child class to set proper widget.
 */
class FMineTabToolBase : public IMineCustomToolModuleListenerInterface, public TSharedFromThis< FMineTabToolBase >
{
public:
    // IPixelopusToolBase

    virtual void OnStartupModule () override
    {
        this->Initialize ();
        FGlobalTabmanager::Get ()->RegisterNomadTabSpawner (TabName, FOnSpawnTab::CreateRaw (this, &FMineTabToolBase::SpawnTab))
            .SetGroup (FMineToolEditor::Get ().GetMenuRoot ())
            .SetDisplayName (TabDisplayName)
            .SetTooltipText (ToolTipText);
    };

    virtual void OnShutdownModule () override
    {
        FGlobalTabmanager::Get ()->UnregisterNomadTabSpawner (TabName);
    };

    // In this function set TabName/TabDisplayName/ToolTipText
    virtual void Initialize () {};
    virtual TSharedRef<SDockTab> SpawnTab (const FSpawnTabArgs &TabSpawnArgs)
    {
        return SNew (SDockTab);
    };

    virtual void MakeMenuEntry (FMenuBuilder &menuBuilder)
    {
        FGlobalTabmanager::Get ()->PopulateTabSpawnerMenu (menuBuilder, TabName);
    };
    virtual ~FMineTabToolBase () {};

protected:
    FName TabName;
    FText TabDisplayName;
    FText ToolTipText;
};