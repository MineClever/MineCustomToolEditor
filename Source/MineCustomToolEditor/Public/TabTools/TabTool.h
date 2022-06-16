#pragma once
#include "MineTabToolBase.h"
#include "TabToolPanel.h"

/*
Let’s look at TabTool class first, it is inherited from TabToolBase defined above.
We set tab name, display name and tool tips in Initialize function,
and prepare the panel in SpawnTab function.Note here we send the tool object itself as a parameter when creating the panel.
This is not necessary,
but as an example how you can pass in an object to the widget.

This tab tool is added in "Section 2" in the custom menu.
*/
class TabTool : public FMineTabToolBase
{
public:
    virtual ~TabTool () {}
    virtual void OnStartupModule () override;
    virtual void OnShutdownModule () override;
    virtual void Initialize () override;
    virtual TSharedRef<SDockTab> SpawnTab (const FSpawnTabArgs &TabSpawnArgs) override;
};