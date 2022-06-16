#include "TabTools/TabTool.h"

void TabTool::OnStartupModule ()
{
    FMineTabToolBase::OnStartupModule ();
    FMineToolEditor::Get ().AddMenuExtension (FMenuExtensionDelegate::CreateRaw (this, &TabTool::MakeMenuEntry), FName ("Section_2"));
}

void TabTool::OnShutdownModule ()
{
    FMineTabToolBase::OnShutdownModule ();
}

void TabTool::Initialize ()
{
    TabName = "TabTool";
    TabDisplayName = FText::FromString ("Tab Tool");
    ToolTipText = FText::FromString ("Tab Tool Window");
}

TSharedRef<SDockTab> TabTool::SpawnTab (const FSpawnTabArgs &TabSpawnArgs)
{
    TSharedRef<SDockTab> SpawnedTab = SNew (SDockTab)
        .TabRole (ETabRole::NomadTab)
        [
            SNew (TabToolPanel)
            .Tool (SharedThis (this))
        ];

    return SpawnedTab;
}