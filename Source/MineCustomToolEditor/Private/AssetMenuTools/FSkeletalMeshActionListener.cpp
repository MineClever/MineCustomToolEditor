#include <AssetMenuTools/FSkeletalMeshActionListener.h>
#include <AssetMenuTools/TMineContentBrowserExtensions_SelectedAssets_Base.h>
#include "PackageTools.h"
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include "AssetMenuTools/TAssetsProcessorFormSelection.hpp"
#include "Rendering/SkeletalMeshRenderData.h"


#define LOCTEXT_NAMESPACE "FSkeletalMeshActionsListener"


/* Processor implementation */
namespace FSkeletalMeshProcessor_AutoSet_Internal
{
    using namespace MineFormatStringInternal;

    class FSkeletalMeshProcessor_AutoSet : public TAssetsProcessorFormSelection_Builder<LocAssetType>
    {
    public:
        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {
            TArray<UObject *> ObjectToSave;
            for (auto SkIt = Assets.CreateConstIterator (); SkIt; ++SkIt) {
                auto const SkMesh = *SkIt;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *SkMesh->GetPathName ());
                SkMesh->Modify ();
                // Do jobs
                for (int32 LodId=0; LodId < SkMesh->GetLODInfoArray().Num();++LodId)
                {
                    FSkeletalMeshLODInfo* const LODInfoPtr = SkMesh->GetLODInfo(LodId);
                    if (LODInfoPtr) {
                        FSkeletalMeshBuildSettings BuildOptions = LODInfoPtr->BuildSettings;

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

    /**
     * @brief 
     */
    class FSkeletalMeshProcessor_AbcClothBindToMatSlots : public TAssetsProcessorFormSelection_Builder<LocAssetType>
    {
        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {
            TArray<UObject *> ObjectToSave;
            uint16 const LodId = 0;

            for (auto SkIt = Assets.CreateConstIterator (); SkIt; ++SkIt) {
                auto const SkMesh = *SkIt;
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *SkMesh->GetPathName ());
                SkMesh->Modify ();
                // Do jobs
                    // Found Material Name on Section
                TArray<FSkeletalMaterial> AllMats = SkMesh->GetMaterials ();

                // get sections number
                int SectionsNum = 0;
                if (SkMesh->GetResourceForRendering () && SkMesh->GetResourceForRendering ()->LODRenderData.Num () > 0) {
                    
                    TIndirectArray<FSkeletalMeshLODRenderData> &LodRenderData = SkMesh->GetResourceForRendering ()->LODRenderData;
                    if (LodRenderData.IsValidIndex (LodId)) {
                        SectionsNum = LodRenderData[LodId].RenderSections.Num ();
                    }
                    for (int SectionId = 0; SectionId < SectionsNum; ++SectionId) {
                        uint16 const CurSectionMatId = LodRenderData[LodId].RenderSections[SectionId].MaterialIndex;
                        FName CurMatSlotName = AllMats[CurSectionMatId].MaterialSlotName;
                        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("MatName as %s"), *CurMatSlotName.ToString());
                    }
                }

                // SkMesh->Build ();
                ObjectToSave.Add (SkMesh);
            }
            UPackageTools::SavePackagesForObjects (ObjectToSave);
        }
    };
}

/* Command Info */
namespace FSkeletalMeshActionsMenuCommandsInfo_Internal
{
    using namespace FSkeletalMeshProcessor_AutoSet_Internal;
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
            // 0
            FORMAT_COMMAND_INFO (0,
                "Auto set SkeletalMesh",
                "Auto set configs for selected SkeletalMesh assets.",
                FSkeletalMeshProcessor_AutoSet
            );


            // END

        }
    };
    // Init Context Fast look up name
    FName MineAssetCtxMenuCommandsInfo::MenuCtxName = TEXT ("MineUSkeletalMeshAssetCtxMenu");

}

/* Menu Extension */
namespace  FSkeletalMeshActionsMenuExtension_Internal
{
    using namespace FSkeletalMeshActionsMenuCommandsInfo_Internal;
    using namespace TMineContentBrowserExtensions_SelectedAssets_Internal;

    /* Extension to menu */
    class FMineContentBrowserExtensions_SelectedAssets : public TMineContentBrowserExtensions_SelectedAssets_Base<LocAssetType, MineAssetCtxMenuCommandsInfo>
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

    };
};


/* Load to module */
namespace FSkeletalMeshMenuActionsListener_Internal
{
    using namespace FSkeletalMeshActionsMenuExtension_Internal;

    // Important : Init Extension Class
    TSharedRef<TMineContentBrowserExtensions_SelectedAssets_Base<LocAssetType, MineAssetCtxMenuCommandsInfo>>
        FSkeletalMeshMenuActionsListener::MenuExtension = 
            MakeShareable (new FMineContentBrowserExtensions_SelectedAssets);

    void FSkeletalMeshMenuActionsListener::InstallHooks ()
    {
        // Push Log
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install %s Asset Menu Hook"), *FMineContentBrowserExtensions_SelectedAssets::AssetTypeName.ToString());

        // register commands
        MineAssetCtxMenuCommandsInfo::Register ();

        // Declare Delegate 
        ContentBrowserExtenderDelegate =
            FContentBrowserMenuExtender_SelectedAssets::CreateSP (
                MenuExtension,
                &FMineContentBrowserExtensions_SelectedAssets::OnExtendContentBrowserAssetSelectionMenu
            );

        // Get all content module delegates
        TArray<FContentBrowserMenuExtender_SelectedAssets> &CBMenuExtenderDelegates = GetExtenderDelegates ();

        // Add The delegate of mine
        CBMenuExtenderDelegates.Add (ContentBrowserExtenderDelegate);

        // Store handle
        ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last ().GetHandle ();
    }
}


#undef LOCTEXT_NAMESPACE