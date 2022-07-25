#include <SequencerExtension/SequencerBaseMenuAction.h>

#include "AssetCreateHelper/FMineStringFormatHelper.h"


#define LOCTEXT_NAMESPACE "FMineSequencerBaseMenuAction"

// Helper functions
namespace FMineSequencerBaseMenuAction_Helper_Internal
{
    template<typename TObjType>
    bool HasValidType (TArray<UObject *> InUObject, const bool bCanCast=false)
    {
        bool bCurrentType = false;
        bool bCanCastType = false;
        for (auto ObjIt = InUObject.CreateConstIterator();ObjIt;++ObjIt)
        {
            if (bCurrentType) break;
            auto const CurObj = *ObjIt;
            bCanCastType = bCanCast ? (Cast<TObjType> (CurObj) != nullptr) : false;
            bCurrentType = CurObj->StaticClass ()->GetFName () == TObjType::StaticClass ()->GetFName () || bCanCastType;
        }
        return bCurrentType;
    }


    class FTestAction
    {
    public:
        static void RunTest ()
        {
            FString const TempDebugString = TEXT ("Do Test to Sequencer!");
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"), *TempDebugString);
            GEngine->AddOnScreenDebugMessage (-1, 5.f, FColor::Blue, *TempDebugString);
        }
    };

}

// UI Command Class
namespace FMineSequencerBaseMenuAction_Internal
{
    class FMineSequencerBaseMenuAction_CommandsInfo : public TCommands<FMineSequencerBaseMenuAction_CommandsInfo>
    {
#define COMMONDINFO_CTX_NAME "FMineSequencerBaseMenuAction"
    public:

        /* INIT */
        FMineSequencerBaseMenuAction_CommandsInfo ()
            : TCommands<FMineSequencerBaseMenuAction_CommandsInfo> (
                TEXT (COMMONDINFO_CTX_NAME), // Context name for fast lookup
                FText::FromString(TEXT (COMMONDINFO_CTX_NAME)), // Context name for displaying
                NAME_None,   // No parent context
                FEditorStyle::GetStyleSetName () // Icon Style Set
                )
        {
        }

        virtual void RegisterCommands () override
        {
            /**
             * @brief: Add UI_COMMAND fastly
             * @param ID : Int
             * @param NAME : "TEXT"
             * @param TIP : "TEXT"
             */
            #define ADD_UI_COMMAND_INFO(ID,NAME,TIP) \
                TSharedPtr<FUICommandInfo> UICommandInfo_0;\
                UI_COMMAND (UICommandInfo_##ID,NAME,TIP, \
                    EUserInterfaceActionType::Button, FInputChord ()); \
                UICommandInfoArray.Emplace (UICommandInfo_##ID)

            ADD_UI_COMMAND_INFO (0, "Name", "Tip");
        }

    public:
        TArray<TSharedPtr<FUICommandInfo>> UICommandInfoArray;

#undef COMMONDINFO_CTX_NAME
    };
}

// Extension Builder
namespace FMineSequencerBaseMenuAction_Internal
{
    using namespace FMineSequencerBaseMenuAction_Helper_Internal;
    using namespace MineFormatStringInternal;

    class FMineSequenceBaseBarActionExtension
    {
    public:
        static TSharedPtr<FExtender> CreateFExtender ()
        {
            UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s"), TEXT ("Debug: Create Bar Extender"));
            TSharedPtr<FUICommandList> const CommandList = MakeShareable (new FUICommandList);
            MapCommands (CommandList);
            TSharedPtr<FExtender> MenuExtender = MakeShareable (new FExtender ());

            MenuExtender->AddToolBarExtension (
                TEXT ("Curve Editor"),
                EExtensionHook::Before,
                CommandList,
                FToolBarExtensionDelegate::CreateStatic (&FMineSequenceBaseBarActionExtension::CreateBarActionsMenu)
            );

            return MenuExtender;
        }

        static void CreateBarActionsMenu (
            FToolBarBuilder &BarBuilder
        )
        {
            auto BaseMenuAction_CommandInfo = FMineSequencerBaseMenuAction_CommandsInfo::Get ();
            UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s"), TEXT ("Debug: Create Sequencer Bar Button"));
            BarBuilder.BeginSection ("MineSequencerToolBar");
            BarBuilder.AddToolBarButton (BaseMenuAction_CommandInfo.UICommandInfoArray[0],
                NAME_None, TAttribute<FText> (), TAttribute<FText> (),
                FSlateIcon (FEditorStyle::GetStyleSetName (), "Matinee.ToggleCurveEditor")
            );
            BarBuilder.EndSection ();
            BarBuilder.AddSeparator ();
        }

        static void MapCommands (const TSharedPtr<FUICommandList> &CommandList)
        {
            auto BaseMenuAction_CommandInfo = FMineSequencerBaseMenuAction_CommandsInfo::Get ();
            CommandList->MapAction (
                BaseMenuAction_CommandInfo.UICommandInfoArray[0],
                FExecuteAction::CreateStatic (&FTestAction::RunTest),
                FCanExecuteAction ()
            );
        }
    };


    class FMineSequenceBaseCtxMenuActionExtension
    {

    public:
        FMineSequenceBaseCtxMenuActionExtension () {};
        virtual ~FMineSequenceBaseCtxMenuActionExtension () {};

        static TSharedPtr<FExtender> CreateFExtender ()
        {
            TSharedPtr<FUICommandList> const CommandList = MakeShareable (new FUICommandList);
            MapCommands (CommandList);
            TSharedPtr<FExtender> MenuExtender = MakeShareable (new FExtender ());
            MenuExtender->AddMenuExtension (
                TEXT ("Edit"),
                EExtensionHook::After,
                CommandList,
                FMenuExtensionDelegate::CreateStatic (&FMineSequenceBaseCtxMenuActionExtension::CreateActionsSubMenu)
            );
            return MenuExtender;
        }

        static void CreateActionsSubMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            
            const FSlateIcon BaseMenuIcon = FSlateIcon ();
            const FText BaseMenuName = LOCTEXT ("ActionsSubMenuToolName", "Mine Sub Actions");
            const FText BaseMenuTip = LOCTEXT ("ActionsSubMenuToolTip", "Add Specific Material key to Current Object.");
            MenuBuilder.BeginSection (TEXT ("MineSequencerBaseActionsMainMenu"), LOCTEXT ("MineSequencerBaseActionsMainMenu", "Mine Sequencer Actions"));
            MenuBuilder.AddSubMenu (
                BaseMenuName,
                BaseMenuTip,
                FNewMenuDelegate::CreateStatic (&PopulateActionsMenu),
                false,
                BaseMenuIcon,
                true,
                FName (TEXT ("MineSequencerBaseActions"))
            );
            MenuBuilder.EndSection ();
        }

        static void PopulateActionsMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            auto BaseMenuAction_CommandInfo = FMineSequencerBaseMenuAction_CommandsInfo::Get ();
            MenuBuilder.BeginSection (TEXT ("MineSequencerBaseActionsSubMenu"), LOCTEXT ("MineSequencerBaseActionsSubMenu", "SequencerBaseAction"));
            MenuBuilder.AddMenuEntry (BaseMenuAction_CommandInfo.UICommandInfoArray[0]);
            MenuBuilder.EndSection ();
        }

        static void MapCommands (const TSharedPtr<FUICommandList> & CommandList)
        {
            auto BaseMenuAction_CommandInfo = FMineSequencerBaseMenuAction_CommandsInfo::Get ();
            CommandList->MapAction (
                BaseMenuAction_CommandInfo.UICommandInfoArray[0],
                FExecuteAction::CreateStatic(&FTestAction::RunTest),
                FCanExecuteAction ()
            );
        }
    };
}


// Module Loader
namespace FMineSequencerBaseMenuAction_Internal
{
    void FMineSequencerBaseExtensionLoader::OnStartupModule ()
    {

        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"), TEXT("Install Sequencer Menu Hooks"));

        // Register Command
        FMineSequencerBaseMenuAction_CommandsInfo::Register ();

        // Add Base Ctx Action Extender
        TSharedPtr<FExtender> const BaseCtxActionExt = FMineSequenceBaseCtxMenuActionExtension::CreateFExtender();
        CtxExtenderPtrArray.Emplace (BaseCtxActionExt);

        // Add Base Ctx Action Extender
        TSharedPtr<FExtender> const BaseBarActionExt = FMineSequenceBaseBarActionExtension::CreateFExtender();
        BarExtenderPtrArray.Emplace (BaseBarActionExt);
        

        // Add All Extender to manager
        for (auto const ExtenderPtr : CtxExtenderPtrArray)
        {
            ObjBindCtxExtensibilityManager->AddExtender (ExtenderPtr);
        }
        for (auto const ExtenderPtr : BarExtenderPtrArray) {
            ToolBarExtensibilityManager->AddExtender (ExtenderPtr);
        }

    }

    void FMineSequencerBaseExtensionLoader::OnShutdownModule ()
    {
        // Remove All Extender from manager
        for (auto const ExtenderPtr : CtxExtenderPtrArray)
        {
            ObjBindCtxExtensibilityManager->RemoveExtender (ExtenderPtr);
        }
        for (auto const ExtenderPtr : BarExtenderPtrArray) {
            ToolBarExtensibilityManager->RemoveExtender (ExtenderPtr);
        }

    }
}

#undef LOCTEXT_NAMESPACE