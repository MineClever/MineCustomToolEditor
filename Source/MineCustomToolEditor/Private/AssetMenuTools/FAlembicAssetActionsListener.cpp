#include <AssetMenuTools/FAlembicAssetActionsListener.h>
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
	class MineAssetCtxMenuCommandsInfo  final : public TCommands<MineAssetCtxMenuCommandsInfo>, public MineAssetCtxMenuCommands_CommandMap
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
                "Auto set Abc Mat",
                "Auto set Alembic-Cache materials.",
                FAlembicAssetProcessor_AutoSetMat
            );

        }
    };
    // Init Context Fast look up name
    FName MineAssetCtxMenuCommandsInfo::MenuCtxName = TEXT ("MineUSkeletalMeshAssetCtxMenu");

};


#undef LOCTEXT_NAMESPACE