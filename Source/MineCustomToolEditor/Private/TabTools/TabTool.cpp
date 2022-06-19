#include "TabTools/TabTool.h"

void TabTool::OnStartupModule ()
{
    
    /*************************************************
     * Initialize FMineTabToolBase
     * FMineTabToolBase will call this->Initialize
     ************************************************/
    FMineTabToolBase::OnStartupModule ();

    /*************************************************
     * Create Delegate for FMineToolEditor::MenuExtension
     ************************************************/
    FMenuExtensionDelegate &&TabMenuDelegate =
        FMenuExtensionDelegate::CreateRaw (this, &TabTool::MakeMenuEntry);

    FMineToolEditor::Get ().AddMenuExtension (
        TabMenuDelegate,
        FName ("Section_2")
    );
}

void TabTool::OnShutdownModule ()
{
    FMineTabToolBase::OnShutdownModule ();
}

void TabTool::Initialize ()
{
    TabName = "MineBaseTable";
    TabDisplayName = FText::FromString ("Mine Base Table Panel");
    ToolTipText = FText::FromString ("Mine Base Tab Tool Window");
}

TSharedRef<SDockTab> TabTool::SpawnTab (
    const FSpawnTabArgs &TabSpawnArgs
)
{
    TSharedRef<SDockTab> SpawnedTab =
        SNew (SDockTab)
        .TabRole (ETabRole::NomadTab)
        [
            SNew (TabToolPanel)
            .Tool (SharedThis (this))
        ];

    return SpawnedTab;
}