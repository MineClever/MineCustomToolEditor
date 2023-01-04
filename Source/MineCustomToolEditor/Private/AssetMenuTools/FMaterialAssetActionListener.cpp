#include <AssetMenuTools/FMaterialAssetActionListener.h>

#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "FSkeletalMeshActionsListener"

namespace FMaterialProcessor_Internal
{
    class FMaterialProcessor_DumpJson : public TAssetsProcessorFormSelection_Builder<LocAssetType>
    {
    public:
        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {
            for (auto AssetIt = Assets.CreateConstIterator();AssetIt;++AssetIt)
            {
                LocAssetType *const Asset = *AssetIt;
                // Get Clothing Assets
                auto LClothAssets = Asset->GetMeshClothingAssets ();
                // Get Material Slots
                
            }
        }
    };
}


namespace FMaterialMenu_CommandsInfo_Internal
{
    using namespace TMineContentBrowserExtensions_SelectedAssets_Internal;

    class MineAssetCtxMenuCommandsInfo final : public TCommands<MineAssetCtxMenuCommandsInfo>, public MineAssetCtxMenuCommands_CommandMap
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
            using namespace FMaterialProcessor_Internal;
            // 0
            FORMAT_COMMAND_INFO (0,
                "Dump Material Settings",
                "Export Material Settings to Json Files.",
                FMaterialProcessor_DumpJson
            );

            // END

        }
    };
    // Init Context Fast look up name
    FName MineAssetCtxMenuCommandsInfo::MenuCtxName = TEXT ("MineUSkeletalMeshAssetCtxMenu");
}

#undef LOCTEXT_NAMESPACE