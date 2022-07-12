#include <AssetMenuTools/FSkeletalMeshActionListener.h>
#include <AssetMenuTools/TMineContentBrowserExtensions_SelectedAssets_Base.h>
#include "AssetToolsModule.h"
#include "PackageTools.h"
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include "AssetMenuTools/FAssetsProcessorFormSelection.hpp"
#include "AssetCreateHelper/FMinePackageSaveHelper.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/SkeletalMeshEditorData.h"
#include "Rendering/SkeletalMeshModel.h"


#define LOCTEXT_NAMESPACE "FSkeletalMeshActionsListener"
#define LOC_ASSET_TYPE USkeletalMesh

namespace FSkeletalMeshProcessor_AutoSet_Internal
{
    using namespace MineFormatStringInternal;

    class FSkeletalMeshProcessor_AutoSet :public TAssetsProcessorFormSelection_Builder<LOC_ASSET_TYPE>
    {
    public:

        virtual void ProcessAssets (TArray<LOC_ASSET_TYPE *> &Assets) override
        {
            TArray<UObject *> ObjectToSave;
            for (auto SkIt = Assets.CreateConstIterator (); SkIt; ++SkIt) {
                auto *const SkMesh = *SkIt;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *SkMesh->GetPathName ());
                SkMesh->Modify ();
                // Do jobs

                for (int32 LodId=0;LodId< SkMesh->GetLODInfoArray().Num();++LodId)
                {
                    FSkeletalMeshLODInfo* const LODInfoPtr = SkMesh->GetLODInfo(LodId);
                    if (LODInfoPtr) {
                        FSkeletalMeshBuildSettings BuildOptions= LODInfoPtr->BuildSettings;

                        //Adjacency buffer, full precision UV and High precision tangent cannot be change in the re-import options, it must not be change from the original data.
                        BuildOptions.bBuildAdjacencyBuffer = true;
                        BuildOptions.bUseFullPrecisionUVs = true;
                        BuildOptions.bUseHighPrecisionTangentBasis = true;

                        BuildOptions.ThresholdUV = 0.000001f;
                        BuildOptions.ThresholdPosition = 0.000001f;
                        BuildOptions.ThresholdTangentNormal = 0.000001f;
                        BuildOptions.MorphThresholdPosition = 0.0001f;

                        //A degenerate face is a face with two vertex indices that are identical, and therefore no geometric area, and no normal .
                        //Some applications consider degenerate faces illegal, and do not allow them to be created or imported.
                        BuildOptions.bRemoveDegenerates = true;

                        BuildOptions.bUseMikkTSpace = false;
                        BuildOptions.bRecomputeNormals = false;
                        BuildOptions.bRecomputeTangents = false;

                        //Copy all the build option to reflect any change in the setting using the re-import UI
                        LODInfoPtr->BuildSettings = BuildOptions;
                    }
                }

                SkMesh->Build ();
                ObjectToSave.Add (SkMesh);
            }
            UPackageTools::SavePackagesForObjects (ObjectToSave);
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
                "Auto set SkeletalMesh",
                "Auto set configs for selected SkeletalMesh assets.",
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
    class FMineContentBrowserExtensions_SelectedAssets : public TMineContentBrowserExtensions_SelectedAssets_Base<USkeletalMesh>
    {
    public:

        FMineContentBrowserExtensions_SelectedAssets ()
        {
            FMineContentBrowserExtensions_SelectedAssets::InitSubMenu ();
        }

        virtual void InitSubMenu () override
        {

            FNsLocTextDescriptions LSubMenuDescriptions;
            LSubMenuDescriptions.Key = TEXT ("SKMeshActionsSubMenuLabel");
            LSubMenuDescriptions.KeyDescription = TEXT ("Mine SkeletalMesh Asset Actions");
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


/* Load to module */

TSharedRef<TMineContentBrowserExtensions_SelectedAssets_Internal::TMineContentBrowserExtensions_SelectedAssets_Base<LOC_ASSET_TYPE>>
FSkeletalMeshMenuActionsListener::MenuExtension =
    MakeShareable (new FSkeletalMeshActionsMenu_Internal::FMineContentBrowserExtensions_SelectedAssets);

void FSkeletalMeshMenuActionsListener::InstallHooks ()
{
    using namespace FSkeletalMeshActionsMenu_Internal;
    UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install SkeletalMesh Asset Menu Hook"));
    // register commands
    MineAssetCtxMenuCommandsInfo::Register ();

    // Declare Delegate 
    ContentBrowserExtenderDelegate =
        FContentBrowserMenuExtender_SelectedAssets::CreateSP (
            MenuExtension,
            &FMineContentBrowserExtensions_SelectedAssets::OnExtendContentBrowserAssetSelectionMenu
        );


    // Get all content module delegates
    TArray<FContentBrowserMenuExtender_SelectedAssets>
        &CBMenuExtenderDelegates = GetExtenderDelegates ();

    // Add The delegate of mine
    CBMenuExtenderDelegates.Add (ContentBrowserExtenderDelegate);
    ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last ().GetHandle ();
}

#undef LOC_ASSET_TYPE
#undef LOCTEXT_NAMESPACE