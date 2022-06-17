
#pragma once
#include "MineMouduleDefine.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"

class MenuTool : public IMineCustomToolModuleListenerInterface, public TSharedFromThis<MenuTool>
{
public:
    virtual ~MenuTool () {}

    virtual void OnStartupModule () override;
    virtual void OnShutdownModule () override;

    void MakeCppMenuEntry (FMenuBuilder &menuBuilder);
    void MakeSubMenu (FMenuBuilder &menuBuilder);

protected:
    TSharedPtr<FUICommandList> CommandList;

    void MapCommands ();

    // Entry Command functions
    void MenuCommand1 ();
    void MenuCommand2 ();
    void MenuCommand3 ();

    // @brief: Tag Command
    FName TagToAdd;

    // For AddTag fucntion, because it is going to be used for a button, return type have to be FReply.
    FReply AddTag ();
    FText GetTagToAddText () const;
    void OnTagToAddTextCommitted (const FText &InText, ETextCommit::Type CommitInfo);
};