#pragma once
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Docking/SDockableTab.h"
#include "Widgets/Docking/SDockTabStack.h"
#include "Framework/Application/SlateApplication.h"
#include "TabTool.h"
#include "MineCustomToolEditor.h"

/*
 * Now for the pannel:
 * In the construct function we build the slate widget in ChildSlot.
 * Here I’m add a scroll box, with a grey border inside, with a text box inside.
 */
class TabToolPanel : public SCompoundWidget
{
    SLATE_BEGIN_ARGS (TabToolPanel) {}
    SLATE_ARGUMENT (TWeakPtr<class TabTool>, Tool)
    SLATE_END_ARGS ()

    void Construct (const FArguments &InArgs);

protected:
    TWeakPtr<TabTool> tool;
};