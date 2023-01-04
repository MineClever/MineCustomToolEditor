
#include <AssetMenuTools/FAlembicAssetActionsListener.h>
#include "EditorStyleSet.h"
#include "PackageTools.h"


#define LOCTEXT_NAMESPACE "FAlembicAssetActionsListener"



namespace FAlembicAssetProcessor_Internal
{

    class FDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
    {
    protected:
        virtual bool Visit (const TCHAR *FilenameOrDirectory, bool bIsDirectory) override
        {
            if (bIsDirectory) {
                FString TempPath;
                // FPackageName::TryConvertFilenameToLongPackageName (FString (FilenameOrDirectory), TempPath);
                DirectoryArray.AddUnique (FString (FilenameOrDirectory));
            }
            return true;
        }
    public:
        TArray<FString> DirectoryArray;
    };

    class FAlembicAssetProcessor_AutoSetMat : public TAssetsProcessorFormSelection_Builder<LocAssetType>
    {
    public:

        FAlembicAssetProcessor_AutoSetMat () : TAssetsProcessorFormSelection_Builder<LocAssetType> (false)
        {
            // Without SourceControl init
        }

        virtual void ProcessAssets (TArray<LocAssetType *> &Assets) override
        {
            TArray<UObject *> ObjectToSave;

            for (auto const Asset: Assets)
            {
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target is : %s"), *Asset->GetPathName ());
            }

            if (ObjectToSave.Num() >0)
				UPackageTools::SavePackagesForObjects (ObjectToSave);
        }
    };
}


namespace FAlembicAssetActionsMenuCommandsInfo_Internal
{
    using namespace FAlembicAssetProcessor_Internal;
	using namespace TMineContentBrowserExtensions_SelectedAssets_Internal;
	class MineAssetCtxMenuCommandsInfo : public TCommands<MineAssetCtxMenuCommandsInfo>, public MineAssetCtxMenuCommands_CommandMap
	{
    public:

        /* INIT */
        static FName MenuCtxName;
        MineAssetCtxMenuCommandsInfo ()
            : TCommands<MineAssetCtxMenuCommandsInfo> (
                MenuCtxName, // Context name for fast lookup
                FText::FromString (MenuCtxName.ToString ()), // Context name for displaying
                NAME_None,   // No parent context
                FAppStyle::GetAppStyleSetName()//FEditorStyle::GetStyleSetName () // Icon Style Set
                )
        {
        }

        /* Register all commands in this class */
        virtual void RegisterCommands () override
        {
            // 0
            FORMAT_COMMAND_INFO (0,
                "Auto set Abc Mat",
                "Auto set Alembic-Cache materials.",
                FAlembicAssetProcessor_AutoSetMat
            );

        }
    };
    // Init Context Fast look up name
    FName MineAssetCtxMenuCommandsInfo::MenuCtxName = TEXT ("MineUSkeletalMeshAssetCtxMenu");

};

namespace FAlembicAssetMenuActionsListener_Internal
{
    /* Extension to menu */
    class FMineContentBrowserExtensions_SelectedAssets : public TMineContentBrowserExtensions_SelectedAssets_Base<LocAssetType, FAlembicAssetActionsMenuCommandsInfo_Internal::MineAssetCtxMenuCommandsInfo>
    {
    public:

        FMineContentBrowserExtensions_SelectedAssets ()
        {
            FMineContentBrowserExtensions_SelectedAssets::InitSubMenu ();
        }

        virtual void InitSubMenu () override
        {

            FNsLocTextDescriptions LSubMenuDescriptions;
            LSubMenuDescriptions.Key = TEXT ("AlembicActionsSubMenuLabel");
            LSubMenuDescriptions.KeyDescription = TEXT ("Alembic Cache Asset Actions");
            LSubMenuDescriptions.LocTextNameSpace = TEXT (LOCTEXT_NAMESPACE);
            this->SubMenuDescriptions = LSubMenuDescriptions;

            FNsLocTextDescriptions LSubMenuTipDescriptions;
            LSubMenuTipDescriptions.Key = TEXT ("AlembicActionsSubMenuToolTip");
            LSubMenuTipDescriptions.KeyDescription = TEXT ("Type-related actions for Alembic Cache Asset.");
            LSubMenuTipDescriptions.LocTextNameSpace = TEXT (LOCTEXT_NAMESPACE);
            this->SubMenuTipDescriptions = LSubMenuTipDescriptions;

            this->SubMenuInExtensionHookName = FName (TEXT ("AlembicAssetsActions"));
            this->SubMenuIcon = FSlateIcon ();
        }

    };
}


/* Load to module */
namespace FAlembicAssetMenuActionsListener_Internal
{

    // Important : Init Extension Class
    TSharedRef<TMineContentBrowserExtensions_SelectedAssets_Base<LocAssetType, MineAssetCtxMenuCommandsInfo>>
        FSkeletalMeshMenuActionsListener::MenuExtension =
        MakeShareable (new FMineContentBrowserExtensions_SelectedAssets);

    void FSkeletalMeshMenuActionsListener::InstallHooks ()
    {
        // Push Log
        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install %s Asset Menu Hook"), *FMineContentBrowserExtensions_SelectedAssets::AssetTypeName.ToString ());

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