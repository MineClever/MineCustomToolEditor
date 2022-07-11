#include <AssetMenuTools/FSkeletalMeshActionListener.h>
#include <AssetMenuTools/TMineContentBrowserExtensions_SelectedAssets_Base.h>
#include "AssetToolsModule.h"
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include "AssetMenuTools/FAssetsProcessorFormSelection.hpp"

#define LOCTEXT_NAMESPACE "FSkeletalMeshActionsListener"

namespace FSkeletalMeshProcessor_AutoSet_Internal
{
    using namespace MineFormatStringInternal;

    FORCEINLINE void SaveUObjectPackage (UObject *InObject)
    {
        InObject->GetPackage ()->MarkPackageDirty ();
        UPackage::Save (InObject->GetPackage (),
            InObject,
            EObjectFlags::RF_Public | ::RF_Standalone,
            *InObject->GetPackage ()->GetName (),
            GError,
            nullptr,
            true,
            true,
            SAVE_NoError | SAVE_Async
        );
    }

    class FSkeletalMeshProcessor_AutoSet :public TAssetsProcessorFormSelection_Builder<USkeletalMesh>
    {
    public:

        virtual void ProcessAssets (TArray<USkeletalMesh *> &Assets) override
        {
            for (auto SkIt = Assets.CreateConstIterator (); SkIt; ++SkIt) {
                USkeletalMesh *const SkMesh = *SkIt;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *SkMesh->GetPathName ());
            }

        }

    };
}

/* Command Info */
namespace FSkeletalMeshActionsMenuCommandsInfo_Internal
{
    class MineAssetCtxMenuCommandsInfo final : public TCommands<MineAssetCtxMenuCommandsInfo>
    {
    public:

        /* INIT */
        static FName MenuCtxName;
        MineAssetCtxMenuCommandsInfo ()
            : TCommands<MineAssetCtxMenuCommandsInfo> (
                MenuCtxName, // Context name for fast lookup
                FText::FromString (MenuCtxName.ToString ()), // Context name for displaying
                NAME_None,   // No parent context
                FEditorStyle::GetStyleSetName () // Icon Style Set
                )
        {
        }

        /* Register all commands in this class */
        virtual void RegisterCommands () override
        {

            UI_COMMAND (MenuCommandInfo_0,
                "Auto set Tex Format",
                "Auto set format for selected texture assets.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );

        }

    public:
        /* Command Action Objects */
        TSharedPtr<FUICommandInfo> MenuCommandInfo_0;

    };
    FName MineAssetCtxMenuCommandsInfo::MenuCtxName = TEXT ("MineUSkeletalMeshAssetCtxMenu");

}

/* Menu Extension */
namespace  FSkeletalMeshActionsMenu_Internal
{
    using namespace TMineContentBrowserExtensions_SelectedAssets_Internal;
    using namespace FSkeletalMeshActionsMenuCommandsInfo_Internal;
    using namespace FSkeletalMeshProcessor_AutoSet_Internal;

    /* Extension to menu */
    class FMineContentBrowserExtensions_SelectedAssets : TMineContentBrowserExtensions_SelectedAssets_Base<USkeletalMesh>
    {
    public:

        virtual void InitSubMenu () override
        {
            FNsLocTextDescriptions LSubMenuDescriptions;
            LSubMenuDescriptions.Key = TEXT ("SKMeshActionsSubMenuLabel");
            LSubMenuDescriptions.KeyDescription = TEXT ("Mine Texture Asset Actions");
            LSubMenuDescriptions.LocTextNameSpace = TEXT (LOCTEXT_NAMESPACE);
            this->SubMenuDescriptions = LSubMenuDescriptions;

            FNsLocTextDescriptions LSubMenuTipDescriptions;
            LSubMenuTipDescriptions.Key = TEXT ("SKMeshActionsSubMenuToolTip");
            LSubMenuTipDescriptions.KeyDescription = TEXT ("Type-related actions for SkeletalMesh Asset.");
            LSubMenuTipDescriptions.LocTextNameSpace = TEXT (LOCTEXT_NAMESPACE);
            this->SubMenuTipDescriptions = LSubMenuTipDescriptions;

            this->SubMenuInExtensionHookName = FName (TEXT ("MineSkeletalMeshAssetsActions"));
            this->SubMenuIcon = FSlateIcon ();
        }

        virtual void PopulateActionsMenu (FMenuBuilder &MenuBuilder) override
        {
            // Add to Menu
            static const MineAssetCtxMenuCommandsInfo &ToolCommandsInfo = MineAssetCtxMenuCommandsInfo::Get ();
            MenuBuilder.AddMenuEntry (ToolCommandsInfo.MenuCommandInfo_0);
        }

        virtual void MappingCommand(const TSharedPtr<FUICommandList> &CommandList,
            const TArray<FAssetData> &SelectedAssets ) override
        {
            static const MineAssetCtxMenuCommandsInfo &ToolCommandsInfo = MineAssetCtxMenuCommandsInfo::Get ();

            CommandList->MapAction (
                ToolCommandsInfo.MenuCommandInfo_0,
                FExecuteAction::CreateStatic (
                    &ExecuteProcessor,
                    AssetsProcessorCastHelper::CreateBaseProcessorPtr<FSkeletalMeshProcessor_AutoSet> (SelectedAssets)
                ),
                FCanExecuteAction ()
            );
        }
    };

};

#undef LOCTEXT_NAMESPACE