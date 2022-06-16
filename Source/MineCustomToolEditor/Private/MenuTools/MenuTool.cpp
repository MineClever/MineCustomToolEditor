#include "MenuTools/MenuTool.h"
#include "MineCustomToolEditor.h"

#define LOCTEXT_NAMESPACE "MenuTool"

class MenuToolCommands : public TCommands<MenuToolCommands>
{
public:

    /* INIT */
    MenuToolCommands ()
        : TCommands<MenuToolCommands> (
            TEXT ("MenuTool"), // Context name for fast lookup
            FText::FromString ("Example Menu tool"), // Context name for displaying
            NAME_None,   // No parent context
            FEditorStyle::GetStyleSetName () // Icon Style Set
            )
    {
    }

    /* Register all commands in this class */
    virtual void RegisterCommands () override
    {
        UI_COMMAND (MenuCommand1, "MenuCommand_1", "Menu Command Test 1.", EUserInterfaceActionType::Button, FInputGesture ());
        UI_COMMAND (MenuCommand2, "MenuCommand_2", "Menu Command Test 2.", EUserInterfaceActionType::Button, FInputGesture ());
        UI_COMMAND (MenuCommand3, "MenuCommand_3", "Menu Command Test 3.", EUserInterfaceActionType::Button, FInputGesture ());
    }

public:
    /* Command Action Objects */
    TSharedPtr<FUICommandInfo> MenuCommand1;
    TSharedPtr<FUICommandInfo> MenuCommand2;
    TSharedPtr<FUICommandInfo> MenuCommand3;
};

/* Mapping Action to Command in CommandList */
void MenuTool::MapCommands ()
{
    /* Get MenuCommands Class instance */
    const auto ToolCommands = MenuToolCommands::Get ();

    /* Do Mapping ! */
    CommandList->MapAction (
        ToolCommands.MenuCommand1,
        FExecuteAction::CreateSP (this, &MenuTool::MenuCommand1),
        FCanExecuteAction ());

    CommandList->MapAction (
        ToolCommands.MenuCommand2,
        FExecuteAction::CreateSP (this, &MenuTool::MenuCommand2),
        FCanExecuteAction ());

    CommandList->MapAction (
        ToolCommands.MenuCommand3,
        FExecuteAction::CreateSP (this, &MenuTool::MenuCommand3),
        FCanExecuteAction ());
}

/* Add MenuAction entry point for each Action */
void MenuTool::MakeCppMenuEntry (FMenuBuilder &menuBuilder)
{
    /* Get MenuCommands Class instance */
    auto &&ToolCommands = MenuToolCommands::Get ();

    /* Add entry */
    menuBuilder.AddMenuEntry (ToolCommands.MenuCommand1);
    menuBuilder.AddMenuEntry (ToolCommands.MenuCommand2);

    /* Add Sub Menu */
    menuBuilder.AddSubMenu (
        FText::FromString ("Sub Menu"),
        FText::FromString ("This is Example sub menu !"),
        FNewMenuDelegate::CreateSP (this, &MenuTool::MakeSubMenu)
    );

    /* Add Tag Widget */
    TSharedRef<SWidget> AddTagWidget = {
        SNew (SHorizontalBox)
        + SHorizontalBox::Slot ()
        .AutoWidth ()
        .VAlign (VAlign_Center)
        [
            SNew (SEditableTextBox)
            .MinDesiredWidth (50)
            .Text (this, &MenuTool::GetTagToAddText)
            .OnTextCommitted (this, &MenuTool::OnTagToAddTextCommitted)
        ]
        + SHorizontalBox::Slot ()
        .AutoWidth ()
        .Padding (5, 0, 0, 0)
        .VAlign (VAlign_Center)
        [
            SNew (SButton)
            .Text (FText::FromString ("TagOnActor"))
            .OnClicked (this, &MenuTool::AddTag)
        ]
    };

    menuBuilder.AddWidget (AddTagWidget, FText::FromString ("+"));


}

void MenuTool::MenuCommand1 ()
{
    UE_LOG (LogMineCustomToolEditor, Log, TEXT ("MineClever的自定义工具 插槽1"));
}

void MenuTool::MenuCommand2 ()
{
    UE_LOG (LogMineCustomToolEditor, Log, TEXT ("MineClever的自定义工具 插槽2"));
}

void MenuTool::MenuCommand3 ()
{
    UE_LOG (LogMineCustomToolEditor, Log, TEXT ("MineClever的自定义工具 插槽3"));
}

void MenuTool::MakeSubMenu (FMenuBuilder &menuBuilder)
{
    auto &&ToolCommands = MenuToolCommands::Get ();
    menuBuilder.AddMenuEntry (ToolCommands.MenuCommand2);
    menuBuilder.AddMenuEntry (ToolCommands.MenuCommand3);
}

FReply MenuTool::AddTag ()
{
    if (!TagToAdd.IsNone ()) {
        const FScopedTransaction Transaction (FText::FromString ("Add Tag"));

        for (FSelectionIterator It (GEditor->GetSelectedActorIterator ()); It; ++It) {
            AActor *Actor = static_cast<AActor *>(*It);
            if (!Actor->Tags.Contains (TagToAdd)) {
                Actor->Modify ();
                Actor->Tags.Add (TagToAdd);
            }
        }
    }
    return FReply::Handled ();
}

FText MenuTool::GetTagToAddText () const
{
    return FText::FromName (TagToAdd);
}

void MenuTool::OnTagToAddTextCommitted (const FText &InText, ETextCommit::Type CommitInfo)
{
    FString str = InText.ToString ();
    TagToAdd = FName (*str.TrimEnd ());
}

/* Add to Menu Section When loading Module */
void MenuTool::OnStartupModule ()
{
    /* Init  CommandList Member Filed in This class*/
    CommandList = MakeShareable (new FUICommandList);
    /* Register ! Call to  MenuToolCommands().RegisterCommands() */
    MenuToolCommands::Register ();

    this->MapCommands ();

    /* Get Current Editor module instance */
    auto &&MineToolEditor = FMineToolEditor::Get ();

    /* Add all Action into Menu Section */
    MineToolEditor.AddMenuExtension (
        FMenuExtensionDelegate::CreateRaw (this, &MenuTool::MakeCppMenuEntry),
        FName ("Section_1"),
        CommandList);
}

void MenuTool::OnShutdownModule ()
{
    MenuToolCommands::Unregister ();
}


#undef LOCTEXT_NAMESPACE