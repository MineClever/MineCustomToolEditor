#include <SequencerExtension/SequencerBaseMenuAction.h>

#include "AssetCreateHelper/FMineStringFormatHelper.h"


#define LOCTEXT_NAMESPACE "FMineSequencerBaseMenuAction"

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
}


namespace FMineSequencerBaseMenuAction_Internal
{
    using namespace FMineSequencerBaseMenuAction_Helper_Internal;
    using namespace MineFormatStringInternal;

    class FMineSequenceBaseMenuActionExtension
    {
    public:
        static TSharedRef<FExtender> CreateFExtender (const TSharedRef<FUICommandList> InCommandList,const TArray<UObject *> InUObject)
        {
            TSharedPtr<FExtender> MenuExtender = MakeShareable (new FExtender());
            if (HasValidType<USkeletalMesh>(InUObject),true)
            {
                MenuExtender->AddMenuExtension (
                    TEXT ("Spawnable"),
                    EExtensionHook::Before,
                    InCommandList,
                    FMenuExtensionDelegate::CreateStatic (&FMineSequenceBaseMenuActionExtension::CreateActionsSubMenu, InCommandList,  InUObject)
                );
            }

            return MenuExtender.ToSharedRef();
        }

        static void CreateActionsSubMenu (
            FMenuBuilder &MenuBuilder,
            const TSharedRef<FUICommandList> InCommandList,
            const TArray<UObject *> InUObject
        )
        {
            const FSlateIcon BaseMenuIcon = FSlateIcon ();
            const FText BaseMenuName = LOCTEXT ("ActionsSubMenuToolName", "Add Material Key");
            const FText BaseMenuTip = LOCTEXT ("ActionsSubMenuToolTip", "Add Specific Material key to Current Object.");

            MenuBuilder.AddSubMenu (
                BaseMenuName,
                BaseMenuTip,
                FNewMenuDelegate::CreateStatic (&PopulateActionsMenu, InCommandList, InUObject),
                false,
                BaseMenuIcon,
                true,
                FName (TEXT ("MineSequencerBaseActions"))
            );

        }

        static void PopulateActionsMenu (
            FMenuBuilder &MenuBuilder,
            const TSharedRef<FUICommandList> InCommandList,
            const TArray<UObject *> InUObject
        )
        {
            MenuBuilder.BeginSection (TEXT ("MineSequencerBaseActionsSubMenu"), LOCTEXT ("MineSequencerBaseActionsSubMenu", "SequencerBaseAction"));
            FString const TempDebugString = TEXT ("Do Test to Sequencer!");
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"),*TempDebugString);
            GEngine->AddOnScreenDebugMessage (-1, 5.f, FColor::Blue, *TempDebugString);
            MenuBuilder.EndSection ();
        }
    };
}


namespace FMineSequencerBaseMenuAction_Internal
{
    void FMineSequencerBaseExtensionLoader::OnStartupModule ()
    {

        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"), TEXT("Install Sequencer Menu Hooks"));

        // Init
        ExtenderDelegates = FMineSequencerBaseExtensionLoader::GetSequencerExt ()->GetExtenderDelegates ();

        // Create Delegate
        auto BaseMenuActionExtDelegate = FAssetEditorExtender::CreateStatic (&FMineSequenceBaseMenuActionExtension::CreateFExtender);

        // Add to Delegates
        ExtenderDelegates.Emplace (BaseMenuActionExtDelegate);

        // Add to Handles to remove
        FDelegateHandle BaseMenuActionExtDelegateHandle = ExtenderDelegates.Last().GetHandle();
        ExtenderHandles.Emplace (BaseMenuActionExtDelegateHandle);

    }

    void FMineSequencerBaseExtensionLoader::OnShutdownModule ()
    {
        // Remove Delegates
        for (auto Handle: ExtenderHandles)
        {
            ExtenderDelegates.RemoveAll (
                [&](const FAssetEditorExtender &Delegate)->bool {
                    return Delegate.GetHandle() == Handle;
                }
            );
        }
    }
}

#undef LOCTEXT_NAMESPACE