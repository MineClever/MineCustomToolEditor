#include <SequencerExtension/SequencerBaseMenuAction.h>
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include <AssetCreateHelper/FMinePackageToObjectHelper.hpp>
#include "LevelSequence.h"
#include "ISequencer.h"
#include "ILevelSequenceEditorToolkit.h"
#include "MovieScene.h"
#include "Tracks/MovieScenePrimitiveMaterialTrack.h"


#define LOCTEXT_NAMESPACE "FMineSequencerBaseMenuAction"

// Helper Functions
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

    FORCEINLINE static UObject *FGuidToUObject (const FGuid &Guid)
    {
        return FUniqueObjectGuid (Guid).ResolveObject ();
    }

    static TArray<UObject *> FGuidToUObject (const TArray<FGuid> &GuidArray)
    {
        TArray<UObject *> ObjectArray;
        for (FGuid Guid : GuidArray) {
            ObjectArray.Emplace (FGuidToUObject (Guid));
        }
        return ObjectArray;
    }

    FORCEINLINE static void GetCurrentEditedAssetsObject (TArray<UObject *> &RefAssets)
    {
        RefAssets.Empty ();
        UAssetEditorSubsystem *&&LEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();
        RefAssets = LEditorSubsystem->GetAllEditedAssets ();
    }

    struct FSequencerHelperFunctions
    {

        /**
         * @brief : found sequence in editing, and GUID of the selections
         * @param SequencerEditor: Access Sequence in Editor
         * @return : sequence in current sequencer editor
         */
        static ULevelSequence* GetFocusSequence (TSharedPtr<ISequencer> &SequencerEditor)
        {
            // Find all Assets opened in Editor
            UAssetEditorSubsystem * &&LEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();
            TArray<UObject *> Assets = LEditorSubsystem->GetAllEditedAssets ();

            for (UObject *Asset : Assets) {
                // Find Sequencer Editor by Asset
                IAssetEditorInstance * &&AssetEditor = LEditorSubsystem->FindEditorForAsset (Asset, false);
                ILevelSequenceEditorToolkit * &&LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit *>(AssetEditor);

                if (LevelSequenceEditor) {
                    // The LevelSequence Object
                    ULevelSequence *LevelSeq = Cast<ULevelSequence> (Asset);
                    
                    // Get Current Level Sequencer FSequencer Tool
                    SequencerEditor = LevelSequenceEditor->GetSequencer();

                    return LevelSeq;
                }
            }
            return nullptr;
        }

    };

    class FTestAction
    {
    public:
        static void RunTest ()
        {
            FString const TempDebugString = TEXT ("Do Test to Sequencer!");
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"), *TempDebugString);
            GEngine->AddOnScreenDebugMessage (-1, 5.f, FColor::Blue, *TempDebugString);

            /* Path to Proxy material */
            static FString const ProxyMatPath = TEXT ("/Game/PalTrailer/MaterialLibrary/Base/Charactor/CFX_Material/Mat_Daili_Inst");
            static FString const DefaultWorldMatPath = TEXT ("/Engine/EngineMaterials/WorldGridMaterial");

            /* Load Mat to Object */
            UObject* ProxyMaterial = MinePackageLoadHelper::LoadAsset(ProxyMatPath);
            
            if (!IsValid(ProxyMaterial))
            {
                /* Load WorldDefault Material to replace */
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to Load Default World Material;\n"));
                ProxyMaterial = MinePackageLoadHelper::LoadAsset (DefaultWorldMatPath);
                if (!IsValid (ProxyMaterial))
                {
                    UE_LOG (LogMineCustomToolEditor, Error, TEXT ("Cant Load Default World Material !!!;\n"));
                    return;
                }
            }

            // Find Sequence
            TSharedPtr<ISequencer> SequencerEditor;
            ULevelSequence* const LevelSequence = FSequencerHelperFunctions::GetFocusSequence(SequencerEditor);

            if (LevelSequence!=nullptr)
            {
                // Tool
                TArray<FGuid> BindingsGuid;
                SequencerEditor->GetSelectedObjects (BindingsGuid);
                UMovieScene* SequencerMovieScene = LevelSequence->GetMovieScene();

                // Find Binding in Current Sequence
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Sequence is %s;\n"), *LevelSequence->GetName ());
                for (FGuid Guid : BindingsGuid) {
                    FMovieSceneBinding* const Binding = SequencerMovieScene->FindBinding(Guid);
                    if (Binding==nullptr) continue;
                    UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Current Sequence Object %s Has been selected in Sequencer Editor;\n"), *Binding->GetName ());

                    // Make a track to add material switcher

                    /* Check if already valid material switch track here */

                    TArray<UMovieSceneTrack*> CurBindingTracks = Binding->GetTracks();
                    for(UMovieSceneTrack* const Track : CurBindingTracks)
                    {
                        UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Current Track Info :%s, %s;\n"), *Track->GetDisplayName().ToString(), *Track->GetClass ()->GetName ());
                        if (Track->GetClass ()->GetFName () == UMovieScenePrimitiveMaterialTrack::StaticClass()->GetFName())
                        {
                            UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Current Track Has MaterialTrack;\n"));
                        }
                    }
                }

            }
        }
    };

}

// Asset Processor Class
namespace FMineSequencerBaseMenuAction_Internal
{
    class FMineSequencerAction_SetHiddenProxyMatKey
    {
        /* Path to Proxy material */
        static FString HiddenProxyMaterialPath;
    };

    FString FMineSequencerAction_SetHiddenProxyMatKey::HiddenProxyMaterialPath = 
        TEXT("/Game/PalTrailer/MaterialLibrary/Base/Charactor/CFX_Material/Mat_Daili_Inst");

}

// UI Command Class
namespace FMineSequencerBaseMenuAction_Internal
{
    using namespace FMineSequencerBaseMenuAction_Helper_Internal;

    struct FMineSequencerAction_CommandsInfo_Base
    {
        TArray<TSharedPtr<FUICommandInfo>> UICommandInfoArray;
    };

    class FMineSequencerBaseMenuAction_CommandsInfo :
        public TCommands<FMineSequencerBaseMenuAction_CommandsInfo>, public FMineSequencerAction_CommandsInfo_Base
    {
    #define CMD_INFO_CTX_NAME "FMineSequencerBaseMenuAction"
    public:

        /* INIT */
        FMineSequencerBaseMenuAction_CommandsInfo ()
            : TCommands<FMineSequencerBaseMenuAction_CommandsInfo> (
                TEXT (CMD_INFO_CTX_NAME), // Context name for fast lookup
                FText::FromString(TEXT (CMD_INFO_CTX_NAME)), // Context name for displaying
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

            // 0
            ADD_UI_COMMAND_INFO (0, "Auto Bind Hidden Mat", "Try to set Hidden Material for current selection in sequencer editor");

            #undef ADD_UI_COMMAND_INFO
        }

        // Mapping FUICommandList
        static void MapCommands (const TSharedPtr<FUICommandList> &CommandList)
        {
            auto BaseMenuAction_CommandInfo = Get ();

            #define BIND_UI_COMMAND_TO_SLOT(ID,FUNC) \
                CommandList->MapAction ( BaseMenuAction_CommandInfo.UICommandInfoArray[##ID##], \
                    FExecuteAction::CreateStatic (&##FUNC##),FCanExecuteAction () \
                );
            // 0
            BIND_UI_COMMAND_TO_SLOT (0, FTestAction::RunTest);

            #undef BIND_UI_COMMAND_TO_SLOT
        }

    #undef CMD_INFO_CTX_NAME
    };
}

// Extension Builder
namespace FMineSequencerBaseMenuAction_Internal
{
    // ToolBar Extension
    template<typename TCmdInfo>
    class FMineSequenceBaseBarActionExtension
    {
    public:
        static TSharedPtr<FExtender> CreateFExtender ()
        {
            UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s"), TEXT ("Debug: Create Bar Extender"));
            TSharedPtr<FUICommandList> const CommandList = MakeShareable (new FUICommandList);
            TCmdInfo::MapCommands (CommandList);
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
            auto BaseMenuAction_CommandInfo = TCmdInfo::Get ();
            UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s"), TEXT ("Debug: Create Sequencer Bar Button"));
            BarBuilder.BeginSection ("MineSequencerToolBar");
            BarBuilder.AddToolBarButton (BaseMenuAction_CommandInfo.UICommandInfoArray[0],
                NAME_None, TAttribute<FText> (), TAttribute<FText> (),
                FSlateIcon (FEditorStyle::GetStyleSetName (), "Matinee.ToggleCurveEditor")
            );
            BarBuilder.EndSection ();
            BarBuilder.AddSeparator ();
        }

    };

    // Context Extension
    template<typename TCmdInfo>
    class FMineSequenceBaseCtxMenuActionExtension
    {

    public:
        FMineSequenceBaseCtxMenuActionExtension () {};
        virtual ~FMineSequenceBaseCtxMenuActionExtension () {};

        static TSharedPtr<FExtender> CreateFExtender ()
        {
            TSharedPtr<FUICommandList> const CommandList = MakeShareable (new FUICommandList);
            TCmdInfo::MapCommands (CommandList);
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
            auto BaseMenuAction_CommandInfo = TCmdInfo::Get ();
            MenuBuilder.BeginSection (TEXT ("MineSequencerBaseActionsSubMenu"), LOCTEXT ("MineSequencerBaseActionsSubMenu", "SequencerBaseAction"));

            // Add CommandsInfo to Menus
            for (int CommandId =0; CommandId < BaseMenuAction_CommandInfo.UICommandInfoArray.Num(); ++CommandId)
            {
                MenuBuilder.AddMenuEntry (BaseMenuAction_CommandInfo.UICommandInfoArray[CommandId]);
            }

            MenuBuilder.EndSection ();
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
        TSharedPtr<FExtender> const BaseCtxActionExt = FMineSequenceBaseCtxMenuActionExtension<FMineSequencerBaseMenuAction_CommandsInfo>::CreateFExtender();
        CtxExtenderPtrArray.Emplace (BaseCtxActionExt);

        // Add Base Ctx Action Extender
        TSharedPtr<FExtender> const BaseBarActionExt = FMineSequenceBaseBarActionExtension<FMineSequencerBaseMenuAction_CommandsInfo>::CreateFExtender();
        BarExtenderPtrArray.Emplace (BaseBarActionExt);
        

        // Add All Extender to manager
        for (auto const ExtenderPtr : CtxExtenderPtrArray) {
            ObjBindCtxExtensibilityManager->AddExtender (ExtenderPtr);
        }
        for (auto const ExtenderPtr : BarExtenderPtrArray) {
            ToolBarExtensibilityManager->AddExtender (ExtenderPtr);
        }

    }

    void FMineSequencerBaseExtensionLoader::OnShutdownModule ()
    {
        // Remove All Extender from manager
        for (auto const ExtenderPtr : CtxExtenderPtrArray) {
            ObjBindCtxExtensibilityManager->RemoveExtender (ExtenderPtr);
        }
        for (auto const ExtenderPtr : BarExtenderPtrArray) {
            ToolBarExtensibilityManager->RemoveExtender (ExtenderPtr);
        }

    }
}

#undef LOCTEXT_NAMESPACE